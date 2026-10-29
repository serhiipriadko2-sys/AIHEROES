"""Check four real Unreal exports using the unchanged local-bridge v0.1 contract.
No networking, model inference, save-game writes, or arbitrary directory ingestion.
"""
from __future__ import annotations
import argparse
import copy
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import sys

from model_request_contract_v1 import ContractError, canonical, validate_request

NAMES = ('seed', 'analyst', 'battle_locked', 'after_forest')

def require(condition: bool, message: str) -> None:
    if not condition:
        raise ValueError(message)

def verify(directory: Path) -> dict:
    require(directory.is_dir() and not directory.is_symlink(), 'Invalid fixture directory')
    snapshots, files = {}, []
    for name in NAMES:
        path = directory / (name + '.json')
        require(path.is_file() and not path.is_symlink() and 0 < path.stat().st_size <= 65536,
                'Missing or invalid bounded fixture: ' + name)
        raw = path.read_bytes()
        snapshots[name] = validate_request(raw)
        files.append({'name': path.name, 'bytes': len(raw), 'sha256': hashlib.sha256(raw).hexdigest()})
    seed, analyst, battle, after = (snapshots[n] for n in NAMES)
    ids = lambda s: {n['id'] for n in s['visible_nodes']}
    orders = lambda s: {(o['order'], o['target_node'], o['cost']) for o in s['legal_orders']}
    require(ids(seed) == {'HQ', 'FOREST', 'ORE'}, 'Initial fog boundary')
    require(seed['protocol_tier'] == 0 and not seed['legal_orders'], 'SEED lock')
    require(analyst['protocol_tier'] == 1 and ('AttackNode', 'FOREST', 2) in orders(analyst), 'Analyst route quote')
    require(analyst['resources'] == {'gold': 160, 'wood': 15, 'ore': 15}, 'Actual upgrade cost')
    require(battle['battle_active'] and not battle['legal_orders'], 'Battle lock')
    require(ids(after) == {'HQ', 'FOREST', 'ORE', 'OUTPOST'} and after['moves_left'] == 2, 'Real forest arrival and fog reveal')
    require(('AttackNode', 'OUTPOST', 1) in orders(after), 'Opened one-move road')
    require(not after['battle_active'], 'Return-to-map phase')
    require(len({s['request_id'] for s in snapshots.values()}) == 4, 'Unique request tokens')
    require(len({s['session_id'] for s in snapshots.values()}) == 1, 'Shared scenario session')
    # Negative controls exercise the same parser, not a permissive imitation.
    rejected = 0
    for case in range(12):
        s = copy.deepcopy(analyst)
        if case == 0: s['extra_internal_field'] = 'rejected'
        elif case == 1: s['selected_node'] = 'RUINS'
        elif case == 2: s['request_id'] = 'not-a-token'
        elif case == 3: s['resources']['gold'] = -1
        elif case == 4: s['moves_left'] = True
        elif case == 5: s['visible_nodes'].append(copy.deepcopy(s['visible_nodes'][0]))
        elif case == 6: s['player_army'][0]['unit_id'] = 'bad"id'
        elif case == 7: s['protocol_tier'] = 0
        elif case == 8: s['battle_active'] = True
        elif case == 9: s['legal_orders'][0]['cost'] = 5
        elif case == 10: s['model_slot_tier'] = 3
        else: s['schema'] = 'unsupported'
        try:
            validate_request(canonical(s))
        except ContractError:
            rejected += 1
        else:
            raise ValueError('Negative control accepted: ' + str(case))
    return {'schema': 'aiheroes.model-request-interop.v1', 'status': 'PASS_SCHEMA_INTEROP',
            'observed_utc': datetime.now(timezone.utc).isoformat(), 'exports': files,
            'real_unreal_exports': 4, 'negative_controls_rejected': rejected,
            'windows_execution': sys.platform == 'win32', 'python': sys.version,
            'model_inference': False, 'network_calls': 0, 'save_writes': 0}

def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--directory', required=True, type=Path)
    parser.add_argument('--report', required=True, type=Path)
    args = parser.parse_args()
    try:
        result = verify(args.directory)
        with args.report.open('x', encoding='utf-8') as f:
            json.dump(result, f, ensure_ascii=True, indent=2)
            f.write('\n')
        print(json.dumps(result, ensure_ascii=True))
        return 0
    except (OSError, ValueError) as exc:
        print('MODEL_REQUEST_INTEROP_FAIL: ' + str(exc), file=sys.stderr)
        return 1

if __name__ == '__main__':
    raise SystemExit(main())
