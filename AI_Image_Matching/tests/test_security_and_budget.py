"""Shared requirement #5/#7 coverage: opt-in API key on mutating review
endpoints and the AI budget guard."""
import sys
from pathlib import Path

import pytest
from fastapi.testclient import TestClient
from sqlalchemy import create_engine
from sqlalchemy.orm import sessionmaker

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "src"))

from api.dependencies import get_db
from api.main import create_app
from db.models import AiCallLog, Base
from telemetry.budget import BudgetExceededError, assert_budget, budget_cap_usd


@pytest.fixture()
def api(tmp_path):
    engine = create_engine(f"sqlite:///{tmp_path / 'test.db'}")
    Base.metadata.create_all(engine)
    Session = sessionmaker(bind=engine, expire_on_commit=False)
    app = create_app()

    def override_get_db():
        with Session() as s:
            yield s

    app.dependency_overrides[get_db] = override_get_db
    return TestClient(app)


def test_mutating_endpoints_open_when_api_key_unset(api, monkeypatch):
    monkeypatch.delenv("SECURITY_KEY", raising=False)
    response = api.post("/review/suggestions", json={"post_id": 99999})
    # no suggestion exists -> passes auth, fails later with 404 (not 401)
    assert response.status_code == 404


def test_missing_or_wrong_api_key_rejected_401(api, monkeypatch):
    monkeypatch.setenv("SECURITY_KEY", "secret")
    missing = api.post("/review/suggestions", json={"post_id": 1})
    wrong = api.post(
        "/review/suggestions",
        json={"post_id": 1},
        headers={"X-Security-Key": "wrong"},
    )
    assert missing.status_code == 401
    assert wrong.status_code == 401


def test_correct_api_key_passes_auth(api, monkeypatch):
    monkeypatch.setenv("SECURITY_KEY", "secret")
    response = api.post(
        "/review/suggestions",
        json={"post_id": 99999},
        headers={"X-Security-Key": "secret"},
    )
    assert response.status_code == 404  # past auth, hits business logic


class _FakeSession:
    def __init__(self, total):
        self._total = total

    def query(self, *a, **kw):
        return self

    def scalar(self):
        return self._total


def test_budget_cap_from_environment(monkeypatch):
    monkeypatch.setenv("BUDGET_CAP_USD", "2.50")
    assert budget_cap_usd() == pytest.approx(2.50)
    monkeypatch.setenv("BUDGET_CAP_USD", "0")
    assert budget_cap_usd() == 0.0  # disabled
    monkeypatch.setenv("BUDGET_CAP_USD", "not-a-number")
    from telemetry.budget import DEFAULT_BUDGET_CAP_USD

    assert budget_cap_usd() == pytest.approx(DEFAULT_BUDGET_CAP_USD)


def test_assert_budget_allows_under_cap_and_blocks_over():
    session = _FakeSession(total=1.25)
    assert assert_budget(session, additional=1.0) == pytest.approx(1.25)

    over = _FakeSession(total=4.99)
    with pytest.raises(BudgetExceededError):
        assert_budget(over, additional=0.02)