"""DoD §6 Backend #1: database models + required indexes.

Run: py evidence/db_schema_overview.py

Introspects the live dev.db schema and prints every table plus its indexes
and unique constraints (the DoD requires indexed models for images, tags,
embeddings, posts, suggestions, approvals/rejections).
"""
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "src"))

from sqlalchemy import create_engine, inspect
import os
from dotenv import load_dotenv

load_dotenv(ROOT / ".env")

url = os.environ.get("DATABASE_URL", f"sqlite:///{ROOT / 'data' / 'dev.db'}")
if not url.startswith("sqlite"):
    url = f"sqlite:///{ROOT / 'data' / 'dev.db'}"


def main() -> None:
    inspector = inspect(create_engine(url))
    tables = sorted(inspector.get_table_names())
    print(f"tables ({len(tables)}): {', '.join(tables)}\n")

    for table in tables:
        cols = [c["name"] for c in inspector.get_columns(table)]
        print(f"{table} ({len(cols)} cols): {', '.join(cols)}")
        for ix in inspector.get_indexes(table):
            kind = "UNIQUE " if ix.get("unique") else ""
            print(f"    {kind}index {ix['name']} ON ({', '.join(ix['column_names'])})")
        for uq in inspector.get_unique_constraints(table):
            if uq.get("name"):
                print(f"    unique {uq['name']} ON ({', '.join(uq['column_names'])})")
        fks = inspector.get_foreign_keys(table)
        for fk in fks:
            print(f"    FK {fk['constrained_columns']} -> {fk['referred_table']}{fk['referred_columns']}")
        print()


if __name__ == "__main__":
    main()