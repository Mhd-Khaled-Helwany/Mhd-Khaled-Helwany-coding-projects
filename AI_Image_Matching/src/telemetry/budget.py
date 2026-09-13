"""Shared requirements #7: AI cost metered AND capped.

`assert_budget(session, additional)` raises BudgetExceededError before an AI
call when the accumulated ai_call_log spend plus `additional` would exceed
BUDGET_CAP_USD (default 5.00; set 0 to disable).
"""
from __future__ import annotations
import os

from sqlalchemy import func
from sqlalchemy.orm import Session

from db.models import AiCallLog

DEFAULT_BUDGET_CAP_USD = 5.0


class BudgetExceededError(RuntimeError):
    pass


def budget_cap_usd() -> float:
    raw = os.environ.get("BUDGET_CAP_USD", "")
    try:
        cap = float(raw) if raw.strip() else DEFAULT_BUDGET_CAP_USD
    except ValueError:
        cap = DEFAULT_BUDGET_CAP_USD
    return max(0.0, cap)


def spent_usd(session: Session) -> float:
    total = session.query(func.sum(AiCallLog.cost_usd)).scalar()
    return float(total or 0.0)


def assert_budget(session: Session, additional: float = 0.0) -> float:
    """Raise if spent + additional exceeds the cap. Returns current spend."""
    cap = budget_cap_usd()
    if cap <= 0.0:
        return 0.0
    spent = spent_usd(session)
    if spent + additional > cap:
        raise BudgetExceededError(
            f"AI budget exceeded: ${spent:.4f} spent + ${additional:.4f} "
            f"requested > BUDGET_CAP_USD ${cap:.2f}"
        )
    return spent