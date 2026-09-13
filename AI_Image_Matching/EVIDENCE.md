# List of evidence for §6 in the capstone pdf

This document is for the capstone reviewers of the internship to make it easier for them to verify my work.

## AI processing

1. Vision model produces structured output validated against a schema; invalid responses are never trusted

Evidence:

**(a) Real vision-model output is structured JSON** — every tagged image has a validated tag file under `data/tags/`, e.g. `data/tags/bamboo.json`:

```json
{
  "subject": "bamboo",
  "category": "plant",
  "attributes": ["green stalks", "segmented stems", "dense grove", "vertical culms", "natural lighting"],
  "caption": "A dense grove of tall green bamboo stalks standing vertically in natural light.",
  "confidence": 1.0
}
```

**(b) The schema is enforced by Pydantic** — `src/vision/schema.py`: `TagSchema` uses closed enums (`Category`, 7 values; `Subject`, 50 values), `confidence: confloat(ge=0.0, le=1.0)` and `"extra": "forbid"`. `validate_tag_payload()` (`src/vision/schema.py:83-88`) returns `(None, errors)` instead of raising, so callers can discard bad payloads.

Valid file validates clean:

```
python scripts/validate_sample.py
```

output:
```
{'valid': True, 'tag': {'subject': <Subject.red_fox: 'red fox'>, 'category': <Category.animal: 'animal'>, 'attributes': ['orange fur', 'wild', 'forest'], 'caption': 'A red fox standing in a forest', 'confidence': 0.94}}
```

**(c) Invalid responses are rejected, never trusted** — the schema rejects unknown categories, subjects outside the closed 50-value enum, out-of-range confidence and missing fields:

```
python evidence/invalid_tag_rejected.py
```
output:

```
[REJECTED] unknown category: 2 schema error(s)
    - category: Input should be 'animal', 'plant', 'vehicle', 'clothing', 'furniture', 'beverage' or 'electronic device'
    - confidence: Input should be less than or equal to 1
[REJECTED] subject outside closed enum: 1 schema error(s)
    - subject: Input should be 'bamboo', 'bed', ... or 'wolf'
[REJECTED] missing required field: 1 schema error(s)
    - confidence: Field required
```

In the batch job (`scripts/run_batch_tagging.py`, `real_worker`) a payload failing `validate_tag_payload` raises before `persist_tag()` is ever called, so an invalid model response is **never written to disk or DB**; `process_batch` classifies validation failures as non-retryable (`src/vision/processor.py:25-36` markers `validation`/`invalid`/`malformed`) and routes them to the failed list.

Automated coverage: `tests/test_vision_schema.py::test_valid_payload_passes` and `::test_invalid_payload_fails_cleanly` (both green, see Quality section for full-suite output).

2. Low-confidence classifications are flagged instead of accepted

Evidence:

**Flagging mechanism (three places):**
- `TagSchema.needs_review(threshold=0.75)` — `src/vision/schema.py:80-81`
- Stored on ingest: `needs_review = confidence < 0.75` in `image_metadata` (`src/matching/repository.py::upsert_image_metadata`)
- Enforced at matching time: the guard rejects any candidate below threshold with reason `"Tag confidence too low to trust"` (`src/matching/guard.py:71-72`)

```
python evidence/confidence_scan.py
```

output:

```
tag files: 50 | min confidence: 0.3 | max: 1.0 | flagged (<0.75): 1
image_metadata rows with needs_review=1: 1
    - basketball.jpg: confidence=0.3
```

3. Images are processed through a batch background job with retries

Evidence:

**Batch + retry implementation** — `src/vision/processor.py`:
- `process_batch(items, worker, max_retries=3, ...)` (lines 83-171) iterates the manifest, retries only transient failures, and returns `{results, failed, attempts}` per item
- `should_retry_error()` (lines 38-57) retries 429/5xx/timeouts/connection errors; policy/safety refusals and validation errors are **not** retried
- `compute_retry_delay()` (lines 59-81) uses exponential backoff with jitter

Wired into the job: `scripts/run_batch_tagging.py` line 229 — `process_batch(batch, worker, max_retries=2, ...)`.

**(a) Live demo** — a flaky API (two timeouts, then success) is retried until it works; a policy refusal is attempted exactly once and routed to failed:

```
python evidence/retry_demo.py
```

output:

```
succeeded: ['tag-ok: img_flaky.jpg']
attempts:  {'img_flaky.jpg': 3, 'img_refused.jpg': 1}
failed:    ['img_refused.jpg']
```

**(b) Automated coverage:**

`test_process_batch_retries_transient_failures_but_not_refusals` proves both directions: a timeouting item is retried (`attempts["bravo"] == 2`) and succeeds, while a policy refusal is given up on immediately (`attempts["charlie"] == 1`).

4. Vision and embedding costs are tracked per call

Evidence:

**Per-call cost computation from the real API response** — `src/vision/gemini_client.py:56-64` reads `usage_metadata` (prompt/completion token counts) from every Gemini response and computes cost with production rates (`$0.000075/1k input`, `$0.0003/1k output`). Embedding calls do the same with their rates (`scripts/seed_embeddings.py`, `EMBEDDING_COST_PER_1K_INPUT`).

**Every call is recorded and persisted** — `summarize_call_metrics()` (`src/vision/processor.py`) computes the record; `record_call()` emits one log line per call, and `persist_ai_call()` (`src/matching/repository.py`) writes it into the `ai_call_log` table (model, tokens, duration, retries, cost). After any tagging/seeding run:

```
python scripts/report_costs.py
```

output:

```
ai_call_log: 68 attributed calls

  #1    2026-08-23 13:06:57 embedding success  gemini-embedding-001   tokens=16*     retries=0 cost=$0.000000 (image 1)
  #2    2026-08-23 13:06:57 embedding success  gemini-embedding-001   tokens=20*     retries=0 cost=$0.000000 (image 2)
  ...
  #68   2026-08-23 13:07:22 embedding success  gemini-embedding-001   tokens=82*     retries=0 cost=$0.000000 (post/corpus)

total: 68 calls, 2479 tokens, $0.000000
(* token count estimated: the embed endpoint does not report usage)

by call type:
  embedding   68 calls     2479 tokens  $0.000000
```

Honesty notes: costs are $0.00 because the embedding free-tier rate is $0 (vision calls use real rates); token counts for embeddings are a documented chars/4 estimate flagged via the record's `meta` since the endpoint reports no usage — vision calls carry real usage metadata.

**Budget guard** — AI jobs stop before a call once accumulated spend would exceed `BUDGET_CAP_USD` (default $5.00, `0` disables): `src/telemetry/budget.py::assert_budget`, wired into both batch scripts.

Automated coverage: `tests/test_batch_processing.py::test_cost_tracking_summarizes_call_metrics_and_cost` asserts the exact cost math `(120 * 0.000075 + 40 * 0.0003) / 1000`; `tests/test_security_and_budget.py::test_assert_budget_allows_under_cap_and_blocks_over` covers the cap.

## Matching system

1. Image and post embeddings are stored; posts return ranked image suggestions

Evidence:

**(a) Embeddings are stored** — `image_vectors` and `post_vectors` hold one Gemini embedding per image caption / post body:

```
python evidence/ranked_suggestions.py
```

output:

```
stored embeddings: 50 image_vectors ['gemini-embedding-001'], 18 post_vectors ['gemini-embedding-001']

post #1 "The Secret Life of the Red Fox" - top 5 ranked images:
  1. redfox.jpg     subject=red fox    category=animal             similarity=0.8637
  2. wolf.jpg       subject=wolf       category=animal             similarity=0.7869
  3. deer.jpg       subject=deer       category=animal             similarity=0.7838
  4. owl.jpg        subject=owl        category=animal             similarity=0.7716
  5. horse.jpg      subject=horse      category=animal             similarity=0.7700
```

**(b) Posts return ranked suggestions over HTTP** — same data through `GET /posts/{id}/images` (start `fastapi dev src/api/main.py` first):

```
python evidence/http_ranked_suggestions.py
```

output:

```
1 "The Secret Life of the Red Fox" -> result=accepted
  1. redfox.jpg     subject=red fox    similarity=0.8637 guard=accepted
  2. wolf.jpg       subject=wolf       similarity=0.7869 guard=rejected
  3. deer.jpg       subject=deer       similarity=0.7838 guard=rejected
  4. owl.jpg        subject=owl        similarity=0.7716 guard=rejected
  5. horse.jpg      subject=horse      similarity=0.7700 guard=rejected
suggestion: redfox.jpg (red fox)
```

returns each ranked candidate with its guard verdict attached (see Safety layer section for a full transcript).

2. Semantic matching works for equivalent concepts — "red fox" matches "Vulpes vulpes"

Evidence:

The Red Fox post body names the animal **only** by its scientific name; no image caption contains any latin name, so there is zero lexical overlap between query and matched text:

```
python evidence/semantic_equivalence.py
```

output:

```
post #1 body sentence: "As a wild animal, Vulpes vulpes -- its scientific name -- has adapted to everything from dense forests to city parks"
image captions containing a latin name: 0/50 (zero lexical overlap)

[stored vectors] ranking all images against the post embedding:
  best: redfox.jpg -> red fox is rank #1 of 50
    1. redfox.jpg     subject=red fox    similarity=0.8637
    2. wolf.jpg       subject=wolf       similarity=0.7869
    3. deer.jpg       subject=deer       similarity=0.7838

[live embeddings] cosine_similarity(embed("red fox"), embed("Vulpes vulpes")) = 0.9261
  embed("red fox") best-matches stored caption of redfox.jpg (red fox, sim=0.8945)
  embed("Vulpes vulpes") best-matches stored caption of redfox.jpg (red fox, sim=0.8584)
```

Reading: the latin-name post still ranks the red-fox photo #1 out of 50 (`gemini-embedding-001` places the two phrasings at cosine 0.93), and both phrasings independently best-match the same stored caption — equivalent concepts converge on the same image despite different wording.

(The script's part 2 makes two live embedding calls and needs `GEMINI_API_KEY`; without it part 1 still runs offline.)

## Safety layer

1. The mismatch guard rejects incorrect recommendations — the wolf-on-a-fox-post scenario provably fails

Evidence:

The wolf photo ranks #2 by similarity for the Red Fox post. Raising the similarity bar above the fox's 0.8637 via the eval/demo knob forces the guard walk past the fox, exposing the verdict it would give every runner-up:

```
curl -s "http://127.0.0.1:8000/posts/1/images?similarity_threshold=0.99"
```

output:

```
result: no_confident_match | reason: Similarity below threshold
  redfox.jpg   sim=0.8637 guard=rejected: Similarity below threshold
  wolf.jpg     sim=0.7869 guard=rejected: Subject mismatch: expected ['red fox'], detected wolf
  deer.jpg     sim=0.7838 guard=rejected: Subject mismatch: expected ['red fox'], detected deer
  owl.jpg      sim=0.7716 guard=rejected: Subject mismatch: expected ['red fox'], detected owl
  horse.jpg    sim=0.7700 guard=rejected: Subject mismatch: expected ['red fox'], detected horse
```

`wolf.jpg` is explicitly rejected for the fox post — the wolf never reaches a human as a suggestion. At default thresholds the same walk simply stops at the accepted fox before wolf is ever considered.

Automated coverage: `tests/test_guard.py::test_forced_wolf_candidate_rejected_on_fox_post` builds the (fox post, wolf image) pair directly and asserts `rejected` + `"Subject mismatch"` naming both subjects.

2. Rejections include a human-readable explanation

Evidence:

Forcing deliberately wrong pairings through the guard prints one plain-language sentence per rejection type:

```
python evidence/rejection_reasons.py
```

output:

```
wrong category: laptop offered to an animal post
  -> rejected: Category mismatch: expected ['animal'], detected electronic device
wrong subject: wolf offered to the red-fox post
  -> rejected: Subject mismatch: expected ['red fox'], detected wolf
untrustworthy tag: basketball.jpg was tagged at 0.3 confidence
  -> rejected: Tag confidence too low to trust
```

All rejection paths flow through the single source of explanations, `evaluate_candidate()` (`src/matching/guard.py:38-77`); the API attaches the same strings per ranked candidate (`guard` + `reason` fields) and the review API's inspect-why endpoint replays them.

Automated coverage: `tests/test_guard.py::test_category_mismatch_rejected`, `::test_subject_mismatch_rejected`, `::test_confidence_gate_rejects`, `::test_similarity_gate_rejects`.

3. When no image clears the bar, the system answers "no confident match" with reasons

Evidence:

The Chess post names no category or subject any corpus image can satisfy; every candidate is rejected with its reason and the answer is "no confident match" instead of a forced pairing:

```
curl -s http://127.0.0.1:8000/posts/18/images
```

output:

```
no_confident_match
reason: Category mismatch: expected none, detected electronic device
suggestion: None
  gamingconsole.jpg guard=rejected: Category mismatch: expected none, detected electronic device
  laptop.jpg   guard=rejected: Category mismatch: expected none, detected electronic device
  bookshelf.jpg guard=rejected: Category mismatch: expected none, detected furniture
  basketball.jpg guard=rejected: Category mismatch: expected none, detected clothing
  belt.jpg     guard=rejected: Category mismatch: expected none, detected clothing
```

(The Lions of the Serengeti post behaves identically with a subject-mismatch reason.) Automated coverage: `tests/test_guard.py::test_walk_all_rejected_yields_no_confident_match` and `tests/test_api.py::test_chess_post_no_confident_match`.

## Backend

1. Database models for images, tags, embeddings, posts, suggestions, approvals/rejections — with the required indexes

Evidence:

Schema introspection of the live `dev.db` — all six DoD model groups exist (images, tags = `image_metadata`, embeddings = `image_vectors`/`post_vectors`, posts, suggestions, approvals/rejections = `review_decisions`) with indexes on every FK plus status/created_at lookup columns:

```
python evidence/db_schema_overview.py
```

output:

```
tables (9): ai_call_log, alembic_version, image_metadata, image_vectors, images, post_vectors, posts, review_decisions, suggestions

image_metadata (12 cols): id, image_id, category, subject, attributes, caption, confidence, needs_review, ...
    index ix_image_metadata_category ON (category)
    index ix_image_metadata_image_id ON (image_id)
    index ix_image_metadata_subject ON (subject)
    unique uq_image_metadata_image_id ON (image_id)
    FK ['image_id'] -> images['id']
...
suggestions (12 cols): id, post_id, image_id, status, similarity, confidence, subject, category, caption, similarity_threshold, confidence_threshold, created_at
    index ix_suggestions_image_id ON (image_id)
    index ix_suggestions_post_id ON (post_id)
    index ix_suggestions_status ON (status)
    unique uq_suggestions_post_image ON (post_id, image_id)
    FK ['image_id'] -> images['id']
    FK ['post_id'] -> posts['id']

review_decisions (6 cols): id, suggestion_id, decision, reason, reviewer, created_at
    index ix_review_decisions_created_at ON (created_at)
    index ix_review_decisions_suggestion_id ON (suggestion_id)
    FK ['suggestion_id'] -> suggestions['id']
```

(Elided above: `ai_call_log`, `images`, `posts`, `image_vectors`, `post_vectors` — full output shows each with its own indexed FK / unique constraint.) Models are defined in `src/db/models.py`; created by Alembic migration `6f7fbb9c3c59`.

2. API endpoints validated; the review workflow (approve / reject / inspect why) exists

Evidence:

Full review lifecycle over HTTP (start `.venv/bin/fastapi dev src/api/main.py` first). Materialize two suggestions, inspect why, approve one, reject the other — the trail records both:

```
curl -s -X POST http://127.0.0.1:8000/review/suggestions -H "Content-Type: application/json" -d '{"post_id": 1}'
```

output:

```
{"id":1,"post_id":1,"image_id":2,"status":"pending",...,"why":{"result":"accepted","reason":null},"decisions":[]}
```

```
curl -s -X POST http://127.0.0.1:8000/review/suggestions -H "Content-Type: application/json" -d '{"post_id": 2}'
```

output: 

```
{"id":2,"post_id":2,"image_id":28,"status":"pending",...,"why":{"result":"accepted","reason":null},"decisions":[]}
```

```
curl -s "http://127.0.0.1:8000/review/suggestions?status=pending"
```

output:

```
[{"id":1,...,"status":"pending",...},{"id":2,...,"status":"pending",...}]
```

```
curl -s http://127.0.0.1:8000/review/suggestions/1        # inspect why
```

output:

```
{"id":1,...,"why":{"result":"accepted","reason":null},"decisions":[]}
```

```
curl -s -X POST http://127.0.0.1:8000/review/suggestions/1/decision -H "Content-Type: application/json" -d '{"decision": "approved", "reviewer": "alice"}'
```

output:

```
{"id":1,...,"status":"approved",...,"decisions":[{"id":1,"suggestion_id":1,"decision":"approved","reviewer":"alice"}]}
```

```
curl -s -X POST http://127.0.0.1:8000/review/suggestions/2/decision -H "Content-Type: application/json" -d '{"decision": "rejected", "reason": "Wolf does not fit the fox article"}'
```

output:

```
{"id":2,...,"status":"rejected",...,"decisions":[{"id":2,"suggestion_id":2,"decision":"rejected","reason":"Wolf does not fit the fox article"}]}
```

```
curl -s http://127.0.0.1:8000/review/decisions            # the review trail
```

output:

```
[{"id":1,"suggestion_id":1,"decision":"approved","reviewer":"alice"},
 {"id":2,"suggestion_id":2,"decision":"rejected","reason":"Wolf does not fit the fox article"}]
```

Validation — malformed input is rejected with 422, never processed:

```
curl -s -o /dev/null -w "%{http_code}" -X POST http://127.0.0.1:8000/review/suggestions/1/decision -H "Content-Type: application/json" -d '{"decision": "maybe"}'
```

output:

```
422 status code in server logs
```

```
curl -s -o /dev/null -w "%{http_code}" "http://127.0.0.1:8000/review/suggestions?status=bogus"
```

output:

```
422 status code in server logs
```

(Note: the 422 comes from request validation and fires before any database lookup — a *valid* decision on a nonexistent suggestion id returns 404 instead.)

Automated coverage: `tests/test_review_api.py` (9 tests) covers materialization, approve/reject trail, inspect-why, idempotent re-materialization, status filtering, and both 422 cases.

## Quality and documentation

1. Automated tests cover schema validation, mismatch rejection, and matching accuracy

Evidence:

**80 tests across 13 files, all green:**

```
python -m pytest -q
```

output:

```
80 passed, 7462 warnings in 29.90s
```

The three DoD criteria map to dedicated suites:

| Criterion | Suite | What it pins down |
|---|---|---|
| Schema validation | `tests/test_vision_schema.py`, `tests/test_corpus_integrity.py` | valid payload passes; invalid payload (unknown enum value, confidence > 1) is never accepted; **every** corpus tag file re-validated against the schema (Probe 1) |
| Mismatch rejection | `tests/test_guard.py` (19 tests) | every gate individually (category, subject, confidence, similarity), the forced wolf-on-fox pairing, walk-skip behavior, and all-rejected -> no_confident_match |
| Matching accuracy | `tests/test_ranking.py`, `tests/test_eval.py` | fox ranks first + ordering/limits; top-1 precision >= 0.9 and guard precision = 100% on the real seeded corpus |

Supporting suites cover the rest of the pipeline end to end: `tests/test_api.py` (HTTP behavior incl. the chess no-confident-match), `tests/test_review_api.py` (review workflow + validation 422s), `tests/test_security_and_budget.py` (opt-in API key auth, AI budget cap), `tests/test_repository.py`, `tests/test_embeddings.py`, `tests/test_batch_processing.py` (retries + cost math), `tests/test_seed.py`, `tests/test_call_logger.py`.

2. A small labeled evaluation dataset measures top-1 precision — the number is in README.md

Evidence:

- The labeled set is `data/posts.csv`: `expected_subject` / `expected_result` per post (16 with a known correct image, 2 that must produce `no_confident_match`)
- Measured by `py scripts/eval_precision.py`
- **The number is in the README**, section "Evaluation": Top-1 precision **93.8% (15/16)**, guard suggestion precision **100% (18/18)** — see `README.md`
- Pinned by `tests/test_eval.py::test_eval_precision_on_real_seeded_db` (asserts top-1 >= 0.9 and guard == 1.0 on the real Gemini-seeded DB)

3. README with architecture explanation and diagram; submission-pack files from § 11 present

Evidence:

All five §11 files exist at the repo root:

`README.md` contains the system description, the ASCII architecture diagram (section "Architecture"), exact run + seed steps ("Run it"), the eval numbers ("Evaluation"), API reference, and the honest "Limitations" note. `capstone.yaml` provides the evaluator manifest (`run:` / `seed:` / `test:` / `base_url:` / probe endpoints); its `test:` command is exactly the green suite shown in box 1.