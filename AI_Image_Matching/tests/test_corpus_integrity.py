"""DoD §12 Probe 1 postcondition: every tagged image has a schema-valid tag
and at least one low-confidence image is flagged, not guessed.

Run against the real corpus in data/tags; skips on a clean checkout where the
generated tags have not been produced yet.
"""
import glob
import json
import sys
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "src"))

from vision.schema import TagSchema, validate_tag_payload

NEEDS_REVIEW_THRESHOLD = 0.75


def _tag_files() -> list[str]:
    return sorted(glob.glob(str(ROOT / "data" / "tags" / "*.json")))


def test_every_image_has_a_schema_valid_tag():
    files = _tag_files()
    if not files:
        pytest.skip("data/tags not generated yet (run scripts/run_batch_tagging.py)")

    assert len(files) >= 50, f"expected the full corpus tagged, found {len(files)}"
    for path in files:
        payload = json.loads(Path(path).read_text(encoding="utf-8"))
        tag, errors = validate_tag_payload(payload)
        assert tag is not None, f"{path} failed schema validation: {errors}"
        assert isinstance(tag, TagSchema)


def test_at_least_one_low_confidence_image_is_flagged():
    files = _tag_files()
    if not files:
        pytest.skip("data/tags not generated yet (run scripts/run_batch_tagging.py)")

    confidences = {
        Path(p).stem: json.loads(Path(p).read_text(encoding="utf-8"))["confidence"]
        for p in files
    }
    flagged = [s for s, c in confidences.items() if c < NEEDS_REVIEW_THRESHOLD]

    assert flagged, (
        "no low-confidence image in corpus - Probe 1 requires at least one "
        "ambiguous image flagged instead of guessed"
    )
    assert min(confidences.values()) < NEEDS_REVIEW_THRESHOLD