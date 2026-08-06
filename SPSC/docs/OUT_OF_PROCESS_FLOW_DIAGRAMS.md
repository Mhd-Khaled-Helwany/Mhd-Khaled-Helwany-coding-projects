# Out-of-Process Flow Diagrams

This document collects the main flow diagrams for the out-of-process strategy runtime added after step 1.

## 1. Full System Overview

```text
                            HELA SYSTEMET EFTER ÄNDRINGARNA SEDAN STEG 1
                            ===========================================

   +--------------------------------------------------------------------------------------+
   |                         EXTERNA / OFFLINE INPUT-KÄLLOR                               |
   |--------------------------------------------------------------------------------------|
   |  Mock MDH / Mock OEH / Mock Central / riktiga adaptrar                               |
   |  - market data                                                                        |
   |  - order responses                                                                    |
   |  - login / wallet / position / account-status                                         |
   +--------------------------------------------------------------------------------------+
                                      |
                                      | UDS / WSS / befintliga adapter-paths
                                      v
   +--------------------------------------------------------------------------------------+
   |                    BESTÅENDE SERVERDELAR I HUVUDPROCESSEN                             |
   |--------------------------------------------------------------------------------------|
   |  ApiManager + handlers + reactor-integration                                          |
   |  EpollReactor                                                                         |
   |  StrategyEngine                                                                       |
   +--------------------------------------------------------------------------------------+
                                      |
                                      | callbacks / events in iour-exchange
                                      v
   +--------------------------------------------------------------------------------------+
   |                                 StrategyEngine                                        |
   |--------------------------------------------------------------------------------------|
   |  - skapar strategier                                                                  |
   |  - väljer runtime per strategi                                                        |
   |  - dränerar actions tillbaka från runtime                                              |
   |  - övervakar worker-processer                                                         |
   +--------------------------------------------------------------------------------------+
                 |                                                          |
                 | in-process                                                | out-of-process
                 v                                                          v
   +--------------------------------------+                 +--------------------------------------+
   | InProcessStrategyRuntime             |                 | OutOfProcessStrategyRuntime          |
   |--------------------------------------|                 |--------------------------------------|
   | Strategy kör i samma process         |                 | Strategy kör i separat process      |
   | via InProcessStrategyServices        |                 | via shared memory + eventfd         |
   +--------------------------------------+                 +--------------------------------------+
                                                                               |
                                                                               | skapar
                                                                               | - 2 shared-memory-ringar
                                                                               | - 2 wakeup-fd:er
                                                                               | - worker-process
                                                                               | - pidfd/supervision
                                                                               v
   +------------------------------------------------------------------------------------------------+
   |                                     strategy-worker process                                     |
   |------------------------------------------------------------------------------------------------|
   |  worker-main                                                                                   |
   |  - öppnar IPC-transporten                                                                      |
   |  - bygger WorkerStrategyServices                                                               |
   |  - instansierar strategi via StrategyFactory                                                   |
   |  - kör strategy->start()                                                                       |
   |  - tar emot events                                                                             |
   |  - skickar actions + ready + heartbeat                                                         |
   +------------------------------------------------------------------------------------------------+
```

## 2. Startup Flow

```text
   StrategyEngine::createStrategy(...)
      -> StrategyLifecycle väljer OutOfProcessStrategyRuntime
      -> OutOfProcessStrategyRuntime initierar transport
         - create engine-side rings
         - skapa engineWakeFd + workerWakeFd
      -> fork()
      -> exec(strategy-worker ...)
      -> open pidfd för supervision
      -> awaitingWorkerReady = true

   worker-main
      -> öppnar ringarna
      -> bygger WorkerStrategyServices
      -> StrategyFactory skapar rätt strategi via service-baserad factory
      -> strategy->start()
      -> skickar StrategyActionWorkerReady

   engine
      -> får WorkerReady
      -> markerar worker som uppe
      -> replayar cachad runtime-state
```

## 3. Normal Event and Action Flow

```text
  1. EXTERNT EVENT KOMMER IN
     Mock MDH / OEH / riktig adapter
        -> ApiManager / handlers
        -> StrategyEngine

  2. ENGINE SKICKAR EVENT TILL WORKER
     StrategyEngine
        -> OutOfProcessStrategyRuntime::sendEvent...
        -> StrategyTransportCodec encodar StrategyEvent till bytes
        -> ShmStrategyTransport::sendEvent(...)
        -> ShmSpscRing (engine -> worker)
        -> eventfd_write(workerWakeFd)

  3. WORKER VAKNAR OCH LÄSER
     worker-main poll()
        -> drainWakeups()
        -> ShmStrategyTransport::tryRecvEvent(...)
        -> ShmSpscRing pop
        -> StrategyTransportCodec dekodar bytes -> StrategyEvent
        -> WorkerStrategyServices::handleEvent(...)

  4. STRATEGIN KÖRS
     dispatchStrategyEvent(...)
        -> konkret Strategy / StrategyBase / MockTestStrategy / annan worker-kompatibel strategi
        -> callback som onTopOfBook / onBookUpdate / onPlaceResponse / osv.

  5. STRATEGIN VILL GÖRA NÅGOT
     Strategy-kod
        -> IStrategyServices
        -> WorkerStrategyServices
        -> skapar StrategyAction
           - subscribe
           - unsubscribe
           - setupExchange
           - place
           - cancel
           - amend
           - replace

  6. WORKER SKICKAR ACTION TILLBAKA
     WorkerStrategyServices
        -> ShmStrategyTransport::sendAction(...)
        -> StrategyTransportCodec encodar StrategyAction
        -> ShmSpscRing (worker -> engine)
        -> eventfd_write(engineWakeFd)

  7. ENGINE VAKNAR OCH APPLICERAR ACTION
     EpollReactor / CommandMailbox / StrategyEngine
        -> runtime wakeup-fd blir läsbar
        -> OutOfProcessStrategyRuntime::drainActions()
        -> ShmStrategyTransport::tryRecvAction(...)
        -> decode StrategyAction
        -> StrategyEngine apply action
        -> befintliga ApiManager-paths används för riktiga subscriptions / order calls
```

## 4. Strategy Services Model

```text
   IStrategyServices
      |
      +--> InProcessStrategyServices
      |      - wrappar ApiManager direkt
      |      - används när strategi kör in-process
      |
      +--> WorkerStrategyServices
             - write-anrop blir IPC-actions tillbaka till engine
             - read-anrop läser från lokal worker-cache

   Exempel:
   getLatestTopOfBook()         -> läses ur worker-cache
   getLatestBook()              -> läses ur worker-cache
   getWalletInfo()              -> läses ur worker-cache
   subscribe()                  -> skickas som StrategyActionSubscribe
   place()                      -> skickas som StrategyActionPlace
   setupExchange()              -> skickas som StrategyActionSetupExchange
```

## 5. Shared Memory IPC Layer

```text
   ShmStrategyTransport
      |
      +--> event ring    : engine -> worker
      |
      +--> action ring   : worker -> engine

   Varje ring är en ShmSpscRing:
      [ Header | Slot0 | Slot1 | Slot2 | ... | SlotN ]

   Header innehåller:
      - magic
      - version
      - slotPayloadBytes
      - slotCount
      - slotStride
      - head  -> nästa skrivindex
      - tail  -> nästa läsindex

   Varje slot innehåller:
      - size
      - payload bytes

   Skrivning:
      producer skriver slot->size + payload
      -> head++

   Läsning:
      consumer läser slot->size + payload
      -> tail++
```

## 6. Crash, Hang, and Restart Flow

```text
   NORMAL ÖVERVAKNING
   StrategyEngine-loop
      -> runtime.pollHealth(now)

   KÄLLOR TILL FELUPPTÄCKT
      1. pidfd blir läsbar
         -> worker har dött / exitat / blivit killad
      2. waitpid(WNOHANG)
         -> worker har lämnat
      3. heartbeat timeout
         -> worker lever men verkar hängd
         -> engine skickar SIGKILL

   VID FEL
      -> recordWorkerFailure(...)
      -> restart delay / backoff uppdateras
      -> workerPid städas bort
      -> ny launchWorkerProcess()

   NY WORKER STARTAR
      -> skickar WorkerReady
      -> engine kör replayStateToWorker()
         - senaste account-status
         - senaste login response
         - senaste wallet update
         - senaste position update

   EFTER REPLAY
      -> workerns strategi kör sin startup-logik igen
      -> subscriptions / setupExchange kan skickas igen
```

## 7. Special Control Messages

```text
   WorkerReady
      -> visar att worker har startat och är redo

   WorkerEventAck
      -> enkel ack / debug-kompatibilitet efter events

   WorkerHeartbeat
      -> skickas periodiskt från worker
      -> används för hang-detection
```

## 8. Testing and Benchmark Flow

```text
   Tester
   ------
   ShmSpscRingTests
      -> verifierar create/open/push/pop/wraparound

   ShmStrategyTransportTests
      -> verifierar event/action-riktningar
      -> verifierar wakeups
      -> verifierar backpressure

   OutOfProcessRuntimeTests
      -> verifierar worker-start
      -> verifierar engine -> worker -> engine-flöde
      -> verifierar restart efter SIGKILL / SIGSEGV / SIGSTOP
      -> verifierar heartbeat timeout / restart backoff

   StrategyAutoRegistrationTests
      -> verifierar att worker-kompatibla strategier kan skapas via factory

   Benchmarks
   ----------
   inprocess_baseline_bench
      -> mäter gammal direkt path

   ipc_roundtrip_bench
      -> mäter ny processisolerad IPC path

   run_out_of_process_benchmarks.sh
      -> kör payload x batch-matris
      -> skriver JSONL-resultat
```

## 9. Main Files in the Flow

- `include/engine/ShmSpscRing.hpp`
- `src/engine/ShmSpscRing.cpp`
- `include/engine/ShmStrategyTransport.hpp`
- `src/engine/ShmStrategyTransport.cpp`
- `include/engine/StrategyTransportCodec.hpp`
- `src/engine/StrategyTransportCodec.cpp`
- `include/engine/StrategyRuntime.hpp`
- `src/engine/OutOfProcessStrategyRuntime.cpp`
- `include/engine/StrategyEngine.hpp`
- `src/engine/StrategyEngine.cpp`
- `src/engine/StrategyLifecycle.cpp`
- `include/strategy/StrategyServices.hpp`
- `include/strategy/WorkerStrategyServices.hpp`
- `src/strategy/WorkerStrategyServices.cpp`
- `src/strategy/worker-main.cpp`
- `tests/ShmSpscRingTests.cpp`
- `tests/ShmStrategyTransportTests.cpp`
- `tests/OutOfProcessRuntimeTests.cpp`
- `tests/StrategyAutoRegistrationTests.cpp`
- `benchmarks/ipc_roundtrip_bench.cpp`
- `scripts/run_out_of_process_benchmarks.sh`
```
