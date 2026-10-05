"""Two natural hotseat turns, cumulative league and the next Wrecking round.

Driving actions are placeholders for real keyboard transitions recorded from
the original. Ports replay those transitions; no scores or race state are set.
"""
import json
from pathlib import Path
from multiplayer_results_protocol import PLAN as RETIREMENT_PLAN

PLAN = json.loads(Path(__file__).with_name('natural_multiplayer_ui.json').read_text())
ACTIONS, KEYS = PLAN['actions'], PLAN['keys']
STARTS, OVERS, TABLES = (tuple(PLAN[key]) for key in ('starts', 'overs', 'tables'))
NAMES = ['step00-boot', *[f'step{i:02d}-{key}' for i, key in enumerate(ACTIONS, 1)]]
START_STATES, OVER_STATES = PLAN['start_states'], PLAN['over_states']
SCOPE = PLAN['scope']


def validate_checkpoint(step, value):
    for steps, expected in ((STARTS, START_STATES), (OVERS, OVER_STATES)):
        if step in steps:
            wanted = expected[steps.index(step)]
            if any(value[key] != val for key, val in wanted.items()):
                raise ValueError(f'Wrong natural multiplayer checkpoint {step}: {wanted}')
    if step >= 22 and (value['race_type'], value['race_mode'], value['multi_count']) != (3, 0, 2):
        raise ValueError('Actual two-player Wrecking race required')
    if step in TABLES and (value['level'], value['poly_list']) != (15, RETIREMENT_PLAN['menu']):
        raise ValueError('Actual cumulative multiplayer league required')
    if step == len(ACTIONS) and value['ticks'] <= 0:
        raise ValueError('The next round must actually run')
