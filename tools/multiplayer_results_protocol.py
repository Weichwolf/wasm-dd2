"""Actual fresh two-player multiplayer season route, without engine state writes."""
import json
from pathlib import Path

PLAN = json.loads(Path(__file__).with_name('multiplayer_results_ui.json').read_text())
KEYS = PLAN['load_keys'] + 5*PLAN['round_keys']
STARTS = tuple(PLAN['starts'])
TABLES = tuple(PLAN['tables'])
OVERS = tuple(PLAN['overs'])
