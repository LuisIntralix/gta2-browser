#!/usr/bin/env python3
"""Validate the GTA2 dependency plan (structure, decisions, dependencies, statuses)."""
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PLAN = ROOT / 'docs/orchestration/tasks.json'


def validate():
    plan = json.loads(PLAN.read_text())
    tasks = {task['id']: task for task in plan['tasks']}
    gates = plan['gates']
    assert len(tasks) == len(plan['tasks']), 'Duplicate task IDs'
    assert not set(tasks).intersection(gates), 'Task/gate ID collision'
    assert isinstance(plan['execution_started'], bool)
    assert (ROOT / plan['spec']).is_file(), 'Missing specification'
    config = json.loads((ROOT / plan['decision_config']).read_text())
    decisions_text = (ROOT / plan['decisions']).read_text()
    import re
    decision_ids = set(re.findall(r'^## (D\d{2}) ', decisions_text, re.MULTILINE))
    assert decision_ids == set(config['decision_ids']), 'Decision registry mismatch'
    assert plan['spec_version'] == config['spec_version'], 'Spec/config version mismatch'
    assert config['decision_status'] == 'locked'
    assert len(plan['source_revision']) == 40, 'Expected full source revision'
    required = set(plan['requirements'])
    covered = set()
    graph = {}
    for task_id, task in tasks.items():
        assert task['status'] in ('planned', 'in_progress', 'partial', 'done'), (task_id, 'Unknown status')
        if not plan['execution_started']:
            assert task['status'] == 'planned', (task_id, 'Unexpected live state')
        assert task['kind'] in ('packet', 'epic'), task_id
        assert task['phase'] in plan['phases'], task_id
        assert task['route'] in plan['routes'], task_id
        assert task['review_route'] in plan['routes'], task_id
        assert task['decision_ids'] and set(task['decision_ids']) <= decision_ids, task_id
        assert task['research_policy'], (task_id, 'Missing bounded discovery policy')
        assert task['route'] != 'research', (task_id, 'Broad research route is closed')
        assert task['write_paths'] and task['deliverables'] and task['acceptance'], task_id
        assert len(set(task['depends_on'])) == len(task['depends_on']), task_id
        assert set(task['depends_on']) <= tasks.keys(), (task_id, 'Unknown task')
        assert set(task['requires_gates']) <= gates.keys(), (task_id, 'Unknown gate')
        assert set(task['requirements']) <= required, (task_id, 'Unknown requirement')
        covered.update(task['requirements'])
        if task['kind'] == 'epic':
            assert task['token_limit'] is None, (task_id, 'Expand epic before budgeting')
        else:
            assert 0 < task['token_limit'] <= plan['packet_policy']['max_token_limit_with_scope_justification'], task_id
        for path in task['write_paths']:
            assert not Path(path).is_absolute() and '..' not in Path(path).parts, task_id
            assert not path.startswith(('downloads/', 'research/gta2_re/', 'node_modules/')), (task_id, 'Protected path')
        graph[task_id] = task['depends_on'] + task['requires_gates']
    assert covered == required, ('Requirements without assigned coverage', required - covered)
    for gate_id, gate in gates.items():
        assert gate['tasks'] and set(gate['tasks']) <= tasks.keys(), gate_id
        graph[gate_id] = gate['tasks']
    for name, route in plan['routes'].items():
        assert 'luna' not in route['model'].lower(), ('Disallowed plan route', name)
        assert route['reasoning'] in ('medium', 'high'), name
    visiting, done = set(), set()

    def visit(node):
        assert node not in visiting, ('Dependency/gate cycle', node)
        if node in done:
            return
        visiting.add(node)
        for dependency in graph[node]:
            visit(dependency)
        visiting.remove(node)
        done.add(node)

    for node in graph:
        visit(node)
    assert config['profile']['source_revision'] == plan['source_revision']
    assert config['simulation']['tick_ms'] == config['network']['snapshot_ms'] == 33
    assert config['stack']['client_physics_prediction'] is False
    assert config['stack']['rollback'] is False
    assert config['network']['snapshot_header_bytes'] == sum([4, 2, 2, 4, 4, 4, 4, 2, 2, 4])
    assert config['assets']['header_bytes'] == 8 + 4 + 4
    assert len(config['assets']['pack_magic'].encode('ascii')) == 8
    rng = config['simulation']['rng']
    state, outputs = rng['seed'], []
    for _ in rng['first_five']:
        state = (state * rng['multiplier'] + rng['increment']) & 0xffffffff
        outputs.append((state >> rng['right_shift']) & rng['mask'])
    assert outputs == rng['first_five'], 'Selected RNG fixture is internally inconsistent'
    for channel in config['network']['channels'].values():
        assert not ('maxRetransmits' in channel and 'maxPacketLifeTime' in channel)
    # Read-only checks of local Markdown destinations, not remote availability.
    missing = []
    for doc in (ROOT / 'docs/orchestration').glob('*.md'):
        for target in re.findall(r'\]\(([^)]+)\)', doc.read_text()):
            if target.startswith(('http:', 'https:', '#')):
                continue
            if not (doc.parent / target.split('#')[0]).exists():
                missing.append((doc.name, target))
    assert not missing, ('Broken local documentation links', missing)
    first = [key for key, task in tasks.items() if not graph[key] and task['kind'] == 'packet']
    print(f'Valid: {len(tasks)} tasks, {len(gates)} gates, {len(required)} requirements; no cycles.')
    print(f'{len(decision_ids)} locked decisions mapped to every task; protocol sizes and RNG fixture consistent.')
    from collections import Counter
    counts = Counter(task['status'] for task in tasks.values())
    if plan['execution_started']:
        print('Status:', ', '.join(f'{counts[s]} {s}' for s in ('done', 'partial', 'in_progress', 'planned') if counts[s]))
        print('Next not started:', ', '.join(t for t, task in tasks.items() if task['status'] == 'planned'))
    else:
        print('Initial dispatch candidates:', ', '.join(first))
        print('No tasks started. Epics must be expanded; readiness is not acceptance.')


if __name__ == '__main__':
    validate()
