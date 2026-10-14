#!/usr/bin/env python3
"""Prepares a private preview of the release build as a Claude artifact.

The artifact host wraps the page in its own document and blocks requests to
other sites, so the page is published without its own <!doctype>/<html>/
<head>/<body> wrapper, and with the one-click Internet Archive download
switched off (players pick their GTA2 folder, or the installer file they
downloaded themselves). No game files are included.

Output: build/artifact/gta2/ (index.html plus the files it loads).
"""
import pathlib
import re
import shutil
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
SRC = ROOT / 'build/port/release'
DEST = ROOT / 'build/artifact/gta2'
FILES = ['shell.js', 'renderer.js', 'storage.js', 'audio.js', 'gamefiles.js', 'map.js', 'installer-worker.js',
         'gta2.js', 'gta2.wasm', 'unshield.js', 'unshield.wasm']


def main():
    if not (SRC / 'gta2.wasm').exists():
        print('build the release preset first (scripts/build_port.py --preset release)', file=sys.stderr)
        return 1
    html = (SRC / 'index.html').read_text()
    title = re.search(r'<title>.*?</title>', html, re.S).group(0)
    style = re.search(r'<style>.*?</style>', html, re.S).group(0)
    body = re.search(r'<body>(.*)</body>', html, re.S).group(1)
    body = body.replace('<script type="module" src="shell.js"></script>',
                        '<script>window.GTA2_NO_DIRECT_DOWNLOAD = true;</script>\n'
                        '  <script type="module" src="shell.js"></script>')
    if DEST.exists():
        shutil.rmtree(DEST)
    DEST.mkdir(parents=True)
    (DEST / 'index.html').write_text(f'{title}\n{style}\n{body.strip()}\n')
    for f in FILES:
        shutil.copy2(SRC / f, DEST / f)
    size = sum(p.stat().st_size for p in DEST.iterdir())
    print(f'{DEST}: {len(FILES) + 1} files, {size / 1e6:.2f} MB')
    return 0


if __name__ == '__main__':
    sys.exit(main())
