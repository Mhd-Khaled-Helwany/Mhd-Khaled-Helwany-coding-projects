# Out-of-Process Validation Notes

This repo now contains the pieces needed to validate the out-of-process strategy runtime against the student requirements.

## Failure injection coverage

Implemented test coverage:

- `SIGKILL` restart:
  - `OutOfProcessRuntimeTests::test_out_of_process_runtime_restarts_worker_after_sigkill`
  - `OutOfProcessRuntimeTests::test_strategy_engine_restarts_worker_after_sigkill`
- `SIGSEGV` restart:
  - `OutOfProcessRuntimeTests::test_out_of_process_runtime_restarts_worker_after_sigsegv`
- Hung process (`SIGSTOP`) detection and restart:
  - `OutOfProcessRuntimeTests::test_out_of_process_runtime_restarts_hung_worker_after_sigstop`
- Restart backoff:
  - `OutOfProcessRuntimeTests::test_out_of_process_runtime_schedules_restart_backoff_after_crash`
- Slow consumer / backpressure:
  - `ShmStrategyTransportTests::test_engine_side_backpressure_reports_full_ring`

The runtime also logs:

- worker crash detection
- restart initiated
- restart completed
- heartbeat timeout for hung workers

## Benchmark targets

Available benchmark executables:

- `inprocess_baseline_bench`
- `ipc_roundtrip_bench`

Both emit the required fields:

- `variant_name`
- `payload_size_bytes`
- `batch_size`
- `iterations`
- `warmup_iterations`
- `p50_ns`
- `p90_ns`
- `p99_ns`
- `p99_9_ns`
- `max_ns`
- `cpu_usage_note`
- `cpu_model`
- `kernel_version`

## Recommended run

Build:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DIOUR_ENABLE_BENCHMARKS=ON
cmake --build build -j
```

Run the full matrix:

```bash
bash scripts/run_out_of_process_benchmarks.sh build
```

This writes JSONL output to:

```text
build/out_of_process_benchmarks.jsonl
```

## Minimal recovery semantics now implemented

On worker restart the runtime:

1. re-establishes shared-memory IPC
2. automatically restarts the worker process
3. replays the latest cached account/login/wallet/position events to the worker
4. lets the worker re-run startup logic and re-issue subscriptions / setup actions

This is still prototype-level recovery, not full exchange/order-state reconstruction.
