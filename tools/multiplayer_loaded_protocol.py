"""Actual loaded positive-score multiplayer season, without engine state writes."""
import json
from pathlib import Path

PLAN=json.loads(Path(__file__).with_name('multiplayer_loaded_ui.json').read_text())
KEYS=PLAN['load_keys']+4*PLAN['round_keys']
STARTS=tuple(PLAN['starts'])
OVERS=tuple(PLAN['overs'])
TABLES=tuple(PLAN['tables'])
CAPTURES=tuple(PLAN['captures'])
