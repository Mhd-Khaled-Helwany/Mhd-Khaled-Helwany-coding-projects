from __future__ import annotations
import os
from collections.abc import Iterator
from fastapi import Header, HTTPException
from sqlalchemy.orm import Session
from db.session import make_session_factory

_session_factory = make_session_factory()

def get_db() -> Iterator[Session]:
    """FastAPI dependency yielding a database session per request."""
    with _session_factory() as session:
        yield session

def require_security_key(x_security_key: str | None = Header(default=None)) -> None:
    """Opt-in authorization for mutating endpoints: enforced only when SECURITY_KEY
    is set in the environment. Unset/empty SECURITY_KEY keeps the API open."""
    expected = os.environ.get("SECURITY_KEY")
    if expected and x_security_key != expected:
        raise HTTPException(status_code=401, detail="Invalid or missing X-Security-Key header")