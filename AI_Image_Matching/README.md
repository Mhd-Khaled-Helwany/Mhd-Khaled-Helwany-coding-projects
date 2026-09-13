# flyrank-capstone-image-relevance

Capstone submission for the backend track at FlyRankAI by Mhd Khaled Helwany

An AI Image Understanding & Content Matching Engine: a corpus of images is
tagged by a vision model, embedded, and matched against blog posts with a
safety guard that **refuses wrong pairings instead of forcing them**.

## What the system does

1. **AI processing** — every image runs through a batch job (retries on
   transient errors, exponential backoff, cost tracked per call). The vision
   model returns structured JSON (`subject`, `category`, `attributes`,
   `caption`, `confidence`) that is validated against a strict Pydantic
   schema; invalid responses are never trusted or persisted.
2. **Matching** — each image caption and post body is embedded
   (`gemini-embedding-001`, 768-dim). A post's candidates are ranked by cosine
   similarity.
3. **Safety layer** — a deterministic mismatch guard walks the top 5 ranked
   candidates through four gates (category keywords, subject keywords,
   confidence >= 0.75, similarity >= 0.74), explains every rejection in plain
   language, and answers "no confident match" when nothing clears the bar.
4. **Review workflow** — suggested pairings land in a `suggestions` table;
   humans approve/reject via validated endpoints, and an append-only
   `review_decisions` trail records every decision.

The two guarantees the whole design is built around:

- a red-fox post gets the red-fox photo, never the wolf
  (`Subject mismatch: expected ['red fox'], detected wolf`)
- when nothing fits, the system says so instead of guessing
  (`no_confident_match` with reasons)

## Architecture diagram

```
Images ──(batch job: retries + cost/call)──► Gemini Flash vision
                                                 │ structured output, schema-validated
                                                 ▼
                                      {subject, category, attributes,
                                       caption, confidence}
                                          │                   │
                   embed(caption)         │      tags + confidence
                          │               │                   ▼
                          ▼               │            image_metadata
                    image_vectors         │            (needs_review flag)
                          │               │                   │
Posts ──► embed(body) ► post_vectors      │                   │
                 │                        │                   │
                 ▼                        ▼                   ▼
        GET /posts/{id}/images ──► Similarity Ranking ──► Mismatch Guard
                                  (cosine, top 5)       category/subject keywords
                                                        + confidence + similarity gates
                                                              │
                                        ┌─────────────────────┴───────────────┐
                                        ▼                                     ▼
                               Suggested image                     "No confident match"
                              (ranked, explained)                       + reasons
                                        │
                                        ▼
                        Review API: approve / reject ──► append-only review trail
```

Storage (SQLite + SQLAlchemy/Alembic): `images`, `image_metadata` (the tags),
`image_vectors`, `posts`, `post_vectors`, `suggestions`,
`review_decisions`, `ai_call_log` — all foreign keys indexed.

## Run it

Prerequisites: Python 3.12+, a free Google AI Studio key.

```bash
python -m venv .venv
.venv/bin/pip install -r requirements.txt
cp .env.example .env            # then put your GEMINI_API_KEY in .env
.venv/bin/alembic upgrade head  # create schema
```

Seed (two steps; both call real Gemini APIs):

```bash
# 1) tag every image in data/manifest.csv (batch job with retries)
USE_REAL_MODEL=1 python scripts/run_batch_tagging.py

# 2) load images/posts/tags into SQLite and embed captions + bodies
USE_REAL_MODEL=1 python scripts/seed_embeddings.py
```

Serve:

```bash
fastapi dev src/api/main.py    # http://127.0.0.1:8000/docs
```

Try the demo moments:

```bash
curl -s http://127.0.0.1:8000/posts/1/images   # fox post -> redfox.jpg accepted
curl -s http://127.0.0.1:8000/posts/18/images  # chess post -> no_confident_match + reasons
```

Tests run fully offline (simulated embeddings, no key needed):

```bash
python -m pytest -q
```

## Evaluation

The labeled set in `data/posts.csv` doubles as the evaluation dataset: 16 posts
have a known correct image, and 2 (Lions of the Serengeti, History of Chess)
must produce `no_confident_match`.

| Metric | Result |
|---|---|
| Top-1 precision (raw similarity ranking) | **93.8%** (15/16) |
| Guard suggestion precision | **100%** (18/18) |

Reproduce:

```bash
python scripts/eval_precision.py
```

The single top-1 miss: the garden-flowers post ranks `bush.jpg` first by
similarity, but the guard walks past it to the correct `flower.jpg` — which is
exactly the value the safety layer adds on top of raw retrieval.

## API

| Method & path | Purpose |
|---|---|
| `GET /health` | corpus + embedding counts |
| `GET /posts` | labeled posts |
| `GET /posts/{id}/images` | ranked candidates + guard verdicts + suggestion (`?similarity_threshold=` / `?confidence_threshold=` are eval/demo knobs) |
| `GET /images/{id}/file` | serve stored image bytes |
| `POST /review/suggestions` | run matching for a post, persist pending suggestion (opt-in auth, see below) |
| `GET /review/suggestions?status=` | review admin table |
| `GET /review/suggestions/{id}` | inspect why (fresh guard verdict + decision trail) |
| `POST /review/suggestions/{id}/decision` | approve / reject (validated; bad values -> 422; opt-in auth) |
| `GET /review/decisions` | append-only review trail |

## Cost tracking

Every vision/embedding call is attributed and persisted to the `ai_call_log`
table: model, token count, duration, retries and computed cost. Inspect it
after any seeding/tagging run:

```bash
python scripts/report_costs.py
```

Two honesty notes: embedding calls report `tokens=N*` — the Gemini embed
endpoint does not return usage via this SDK, so token counts are estimated
(chars/4) and flagged as such; their free-tier rate is $0. Vision calls carry
real usage metadata and real rates. A budget guard stops AI jobs before a call
once accumulated spend would exceed `BUDGET_CAP_USD` (default $5.00, `0`
disables).

## The SECURITY_KEY variable (opt-in auth)

`SECURITY_KEY` protects the state-changing review endpoints
(`POST /review/suggestions` and `POST /review/suggestions/{id}/decision`) with
a single shared secret:

- **Where it comes from**: nowhere external — *the user* generates it, e.g.
  `openssl rand -hex 32`, put the value in `.env`, and share it only with
  people who should be allowed to approve/reject suggestions.
- **How it works**: while a non-empty `SECURITY_KEY` is configured, every
  request to those two endpoints must send header `X-Security-Key` with the
  exact value; anything else gets `401` before any business logic runs.
- **When empty or unset** (the shipped default): the check disables itself and
  the API behaves exactly as if the feature did not exist — zero effect on
  requests, tests, or the demo flow.

Do not confuse it with `GEMINI_API_KEY`: that one is an *outbound* credential
your server uses to call Google's models (get it from Google AI Studio);
`SECURITY_KEY` is an *inbound* secret protecting your own API. One authenticates
you to a provider; the other authenticates callers to you.

## Limitations (honest notes)

- **Scale**: brute-force cosine over <= 50 vectors in Python + SQLite. Fine for
  the demo corpus; real scale wants pgvector/ANN and a production ASGI setup
  (this repo serves via `fastapi dev`).
- **Embedding token counts are estimated** (the embed endpoint does not report
  usage); vision calls use real usage data. Costs on the free tier are $0.
- **Keyword-based guard**: category/subject gates scan post text against the
  closed vocabulary, so posts whose wording misses the enum words rely entirely
  on the similarity/confidence gates. English only.
- **Thresholds tuned on this corpus** (0.74 similarity / 0.75 confidence over
  these 50 images); they may not transfer to other domains without re-tuning.
- **Small, self-authored eval set** (18 posts written alongside the corpus), so
  the precision numbers are optimistic versus held-out data.
- **Batch job is a CLI script** with retries — not a scheduled background
  worker queue; no progress UI beyond stdout.
- **Auth is opt-in and minimal**: setting `SECURITY_KEY` protects the mutating
  review endpoints via one header; GET endpoints stay open by design, and
  there are no users/roles.
- Removing an image from `manifest.csv` does not delete its rows from the DB
  (upsert-only seeding); stale rows must be removed manually.

## Submission pack

- `capstone.yaml` — evaluator manifest (run / seed / test / base_url / probes)
- `EVIDENCE.md` — one verifiable proof per Definition-of-Done checkbox
- `BUILDLOG.md` — where AI was used in building this project
- `.env.example` — every environment variable the app needs, with safe placeholder values

