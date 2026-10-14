#!/usr/bin/env python3
"""Reproduce the asset-free, bounded gta2_re portability probe (no downloads).

Requires the pinned checkout, local emsdk 6.0.12, Apple clang++, and Node.
Copies only math source to research/probes/core; never edits upstream.
"""
import difflib
import json
import os
from pathlib import Path
import subprocess
import time

ROOT = Path(__file__).resolve().parents[1]
REPO = ROOT / 'research/gta2_re'
SDK = ROOT / 'research/toolchains/emsdk'
PROBE = ROOT / 'research/probes'
CORE = PROBE / 'core'
OUT = ROOT / 'docs/evidence/gta2-build-audit-20261012/build/reproduction'
PIN = '02de8beeb766644417f195d360eba33af0ee491f'
FILES = ['types.hpp', 'Globals.hpp', 'Function.hpp', 'fix16.hpp', 'fix16.cpp',
         'ang16.hpp', 'ang16.cpp', 'Fix16_Point.hpp', 'Fix16_Point.cpp', 'BitSet32.hpp']
PATCHES = {
    'fix16.hpp': [
        ('Max_41E130(Fix16& a, Fix16& b)', 'Max_41E130(const Fix16& a, const Fix16& b)'),
        ('SquareRoot(Fix16& input)', 'SquareRoot(const Fix16& input)'),
        ('SquareRoot_forced(Fix16& input)', 'SquareRoot_forced(const Fix16& input)'),
        ('SquareRoot_436A70(Fix16& a2)', 'SquareRoot_436A70(const Fix16& a2)'),
    ],
    'fix16.cpp': [('SquareRoot_436A70(Fix16& input)',
                   'SquareRoot_436A70(const Fix16& input) throw()')],
    'ang16.hpp': [('Ang16(Fix16& value, u8 zero)', 'Ang16(const Fix16& value, u8 zero)')],
}


def run(name, command, env=None, required=True):
    start = time.monotonic()
    p = subprocess.run(command, cwd=ROOT, env=env, text=True, capture_output=True)
    record = dict(command=command, returncode=p.returncode,
                  seconds=round(time.monotonic() - start, 4), stdout=p.stdout, stderr=p.stderr)
    (OUT / (name + '.json')).write_text(json.dumps(record, indent=2) + '\n')
    if required and p.returncode:
        raise RuntimeError(f'{name} failed; see {OUT / (name + ".json")}')
    return record


def main():
    assert subprocess.check_output(['git', '-C', str(REPO), 'rev-parse', 'HEAD'], text=True).strip() == PIN
    CORE.mkdir(parents=True, exist_ok=True)
    OUT.mkdir(parents=True, exist_ok=True)
    diff = ''
    for name in FILES:
        original = (REPO / 'Source' / name).read_text()
        updated = original
        for before, after in PATCHES.get(name, []):
            assert updated.count(before) == 1, (name, before)
            updated = updated.replace(before, after)
        (CORE / name).write_text(updated)
        diff += ''.join(difflib.unified_diff(original.splitlines(True), updated.splitlines(True),
                                            fromfile='upstream/' + name, tofile='probe/' + name))
    (OUT / 'math-compatibility.patch').write_text(diff)
    (PROBE / 'windows.h').write_text('// Probe only: these math units call no Windows API.\n')
    (PROBE / 'package.json').write_text('{"private":true,"type":"commonjs"}\n')
    flags = ['-std=c++11', '-O2', '-fms-extensions', '-D__declspec(x)=', '-D__cdecl=',
             '-D__forceinline=inline', '-Wno-ignored-attributes', '-Wno-microsoft-extra-qualification',
             '-I' + str(PROBE), '-I' + str(CORE), str(ROOT / 'scripts/gta2_math_probe.cpp')]
    flags += [str(CORE / name) for name in ['fix16.cpp', 'ang16.cpp', 'Fix16_Point.cpp']]
    native = str(PROBE / 'math-native')
    run('native-build', ['clang++', *flags, '-Wl,-dead_strip', '-o', native])
    native_result = json.loads(run('native-run', [native])['stdout'])
    env = dict(os.environ, EM_CONFIG=str(SDK / '.emscripten'), EM_CACHE=str(SDK / 'upstream/emscripten/cache'))
    run('wasm-build', ['python3', str(SDK / 'upstream/emscripten/em++.py'), *flags,
                      '-sENVIRONMENT=web,node', '-sMODULARIZE=1', '-sEXPORT_NAME=createGta2MathProbe',
                      '-o', str(PROBE / 'math-probe.js')], env=env)
    js = 'require("./research/probes/math-probe.js")({print:console.log,printErr:console.error}).catch(e=>{console.error(e);process.exitCode=1})'
    wasm_result = json.loads(run('wasm-node-run', ['node', '-e', js])['stdout'])
    for result in [native_result, wasm_result]:
        assert result['checks'] == 44577 and result['failures'] == 0
    assert native_result['checksum'] == wasm_result['checksum']
    sanitizer_flags = ['-O1' if f == '-O2' else f for f in flags]
    ubsan = str(PROBE / 'math-ubsan')
    run('ubsan-build', ['clang++', '-fsanitize=undefined', '-fno-sanitize-recover=undefined',
                        *sanitizer_flags, '-Wl,-dead_strip', '-o', ubsan])
    negative = run('ubsan-negative-divide', [ubsan, '--negative-division'], required=False)
    assert negative['returncode'] != 0 and 'left shift of negative value' in negative['stderr']
    (PROBE / 'index.html').write_text('''<!doctype html><html lang="en"><meta charset="utf-8">
<title>GTA2 math reuse probe</title><h1>GTA2 math reuse probe</h1>
<p>Three reconstructed math translation units, adapted for standard C++.
This is not game execution or a frame-rate benchmark.</p><pre id="result">Running WebAssembly…</pre>
<script src="math-probe.js"></script><script>
const lines=[];createGta2MathProbe({print:line=>{lines.push(line);
document.getElementById('result').textContent=lines.join('\\n');},
printErr:line=>{document.getElementById('result').textContent+='\\nERROR: '+line;}})
.catch(e=>{document.getElementById('result').textContent='FAILED: '+e;});</script></html>''')
    summary = dict(revision=PIN, native=native_result, wasm_node=wasm_result,
                   negative_division_ubsan='known failure reproduced',
                   browser='Must be opened separately; this runner does not automate browser execution.')
    (OUT / 'summary.json').write_text(json.dumps(summary, indent=2) + '\n')
    print(json.dumps(summary, indent=2))


if __name__ == '__main__':
    main()
