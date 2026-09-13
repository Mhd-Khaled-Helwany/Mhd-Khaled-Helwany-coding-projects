"""§12 Probe 6: every vision/embedding call attributed with a cost entry.
Run: `py scripts/report_costs.py`
"""
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "src"))

from dotenv import load_dotenv

load_dotenv(ROOT / ".env")

from sqlalchemy import create_engine
from sqlalchemy.orm import sessionmaker
import os
from db.models import AiCallLog

def main() -> int:
    url = os.environ.get("DATABASE_URL", f"sqlite:///{ROOT / 'data' / 'dev.db'}")
    engine = create_engine(url if url.startswith("sqlite") else f"sqlite:///{ROOT / 'data' / 'dev.db'}")
    with sessionmaker(bind=engine)() as session:
        calls = session.query(AiCallLog).order_by(AiCallLog.id).all()

    if not calls:
        print("ai_call_log is empty - run scripts/run_batch_tagging.py or "
              "scripts/seed_embeddings.py first.")
        return 1

    print(f"ai_call_log: {len(calls)} attributed calls\n")
    for c in calls:
        image = f"image {c.image_id}" if c.image_id else "post/corpus"
        estimated = isinstance(c.meta, dict) and c.meta.get("tokens_estimated")
        token_label = f"{c.total_tokens}{'*' if estimated else ''}"
        print(f"  #{c.id:<4} {c.created_at:%Y-%m-%d %H:%M:%S} {c.call_type:<9} {c.status:<8} "
              f"{str(c.model_name):<22} tokens={token_label:<7} retries={c.retry_count} "
              f"cost=${c.cost_usd:.6f} ({image})")

    total_cost = sum(c.cost_usd for c in calls)
    total_tokens = sum(c.total_tokens for c in calls)
    print(f"\ntotal: {len(calls)} calls, {total_tokens} tokens, ${total_cost:.6f}")
    print("(* token count estimated: the embed endpoint does not report usage)")

    by_type = {}
    for c in calls:
        b = by_type.setdefault(c.call_type, {"calls": 0, "tokens": 0, "cost": 0.0})
        b["calls"] += 1
        b["tokens"] += c.total_tokens
        b["cost"] += c.cost_usd
    print("\nby call type:")
    for call_type, b in sorted(by_type.items()):
        print(f"  {call_type:<9} {b['calls']:>4} calls  {b['tokens']:>7} tokens  ${b['cost']:.6f}")
    return 0

if __name__ == "__main__":
    sys.exit(main())