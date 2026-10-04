"""Shared actual result-table route for strict clock/RNG history recording."""
import json
from pathlib import Path

PLAN = json.loads(Path(__file__).with_name('race_results_ui.json').read_text())
KEYS = PLAN['load_keys'] + 3*(PLAN['retire_keys']+['Return','Escape','Return']+
                             PLAN['continuation_keys']+['Return'])
STARTS = tuple(PLAN['starts'])
TABLES = tuple(PLAN['tables'])
OVERS = tuple(PLAN['overs'])
