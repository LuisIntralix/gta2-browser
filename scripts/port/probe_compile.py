#!/usr/bin/env python3
"""Syntax-check every gta2_re library source for wasm32 and summarise errors.

Usage: probe_compile.py [--files a.cpp,b.cpp] [--show N]
Reads the port working copy at research/port-work/core.
"""
import argparse, collections, concurrent.futures, os, re, subprocess, sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
CORE = ROOT / 'research/port-work/core'
EMXX = ROOT / 'research/toolchains/emsdk/upstream/emscripten/em++'
FLAGS = ['-std=gnu++17', '-fshort-wchar', '-mms-bitfields', '-fno-strict-return', '-Wno-c++11-narrowing', '-Wno-address-of-temporary', '-Wno-dynamic-exception-spec', '-fms-extensions', '-fdeclspec', '-fsyntax-only', '-w',
         '-ferror-limit=0', '-include', str(ROOT / 'port/core/compat/port_prefix.h'),
         '-I', str(ROOT / 'port/core/compat'), '-I', str(CORE / 'Source'), '-I', str(CORE),
         '-D_CRT_SECURE_NO_WARNINGS', '-DGTA2_PORT=1']


def compile_one(src):
    r = subprocess.run([str(EMXX), *FLAGS, str(CORE / src)], capture_output=True, text=True)
    return src, r.returncode, r.stderr


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--files')
    ap.add_argument('--show', type=int, default=40)
    ap.add_argument('--log', help='write the full compiler output here')
    a = ap.parse_args()
    files = a.files.split(',') if a.files else (ROOT / 'port/core/sources.txt').read_text().split()
    errs = collections.Counter()
    per_file = {}
    full = []
    with concurrent.futures.ThreadPoolExecutor(os.cpu_count()) as ex:
        for src, rc, err in ex.map(compile_one, files):
            full.append(err)
            lines = [l for l in err.splitlines() if ' error: ' in l]
            per_file[src] = lines
            for l in lines:
                msg = re.sub(r'^.*? error: ', '', l)
                errs[msg] += 1
    if a.log:
        Path(a.log).write_text('\n'.join(full))
    ok = sum(1 for v in per_file.values() if not v)
    print(f'{ok}/{len(files)} files compile cleanly; {sum(errs.values())} errors, {len(errs)} distinct')
    for msg, n in errs.most_common(a.show):
        print(f'{n:5d}  {msg}')
    worst = sorted(per_file.items(), key=lambda kv: -len(kv[1]))[:8]
    print('worst files:', ', '.join(f'{Path(k).name}={len(v)}' for k, v in worst if v))


if __name__ == '__main__':
    main()
