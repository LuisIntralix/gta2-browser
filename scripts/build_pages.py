#!/usr/bin/env python3
"""Builds the static site for GitHub Pages (M1sterMo/gta2) in build/pages.

The site is the release build of the browser port with no game files: in
static mode (config.json {"static": true}) the landing page's Play button
downloads the 2004 freeware installer from the Internet Archive into the
player's browser, unpacks it there and keeps it. Co-op uses the peer-to-peer
transport (WebRTC through public trackers); saved crews need the game server.

Usage: scripts/build_pages.py   (run scripts/build_port.py --preset release first)
"""
import json
import pathlib
import shutil
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
RELEASE = ROOT / 'build/port/release'
SITE = ROOT / 'build/pages'

# What goes out: the page, its scripts and the WebAssembly, licences, images and the vendored
# peer-to-peer library. Never game files, developer tools or build products.
FILES = ('index.html', 'gta2.js', 'gta2.wasm', 'unshield.js', 'unshield.wasm', 'unshield-LICENSE.txt')
SUFFIXES = ('.js',)
DIRS = ('vendor',)  # not img/: game screenshots stay off public pages (AGENTS.md); the hero closes up
GAME_FILE = ('.gmp', '.sty', '.sdt', '.raw', '.wav', '.gxt', '.scr', '.mmp', '.exe', '.dll', '.bik', '.tgx')

README = """# GTA2 in your browser

**Grand Theft Auto 2 (1999), the real game, in a browser tab.** No install. Bring friends.

### ▶ [Play now](https://m1stermo.github.io/gta2/)

- The first time you press Play, the page downloads the official 2004 freeware release of GTA2
  from the [Internet Archive](https://archive.org/details/gta2_rockstar-classics) (346 MB). It
  unpacks it in your browser and keeps it for next time.
- Widescreen, a calmer camera, a full map with waypoints, and smooth motion on high-refresh
  screens.
- Up to 6 players: free roam, **Gang Wars** (crews side with the street gangs) and
  free-for-all, connected browser to browser.

**Host your own copy:** fork this repo, then in **Settings → Pages** choose *Deploy from a
branch*, `main`, `/ (root)`. It goes live at `https://<you>.github.io/gta2/`. Any static host
works too; serve the files as they are.

This repo is the built site only and holds **no game files**. The engine is the
[gta2_re](https://github.com/CriminalRETeam/gta2_re) decompilation, compiled to WebAssembly.

GTA2 is © Rockstar Games. This is an unofficial, non-commercial fan project, not affiliated
with Rockstar Games or Take-Two Interactive.
"""


def main():
    if not (RELEASE / 'gta2.wasm').exists():
        print('build_pages: build the release first (scripts/build_port.py --preset release)', file=sys.stderr)
        return 1
    if SITE.exists():
        keep_git = SITE / '.git'
        for item in SITE.iterdir():
            if item != keep_git:
                shutil.rmtree(item) if item.is_dir() else item.unlink()
    SITE.mkdir(parents=True, exist_ok=True)
    for f in RELEASE.iterdir():
        if f.is_file() and (f.name in FILES or f.suffix in SUFFIXES):
            shutil.copy2(f, SITE / f.name)
    for d in DIRS:
        shutil.copytree(RELEASE / d, SITE / d)
    (SITE / 'config.json').write_text(json.dumps({'static': True}) + '\n')
    (SITE / '.nojekyll').write_text('')
    (SITE / 'README.md').write_text(README)
    shipped = [p for p in SITE.rglob('*') if p.is_file() and '.git' not in p.parts]
    bad = [p for p in shipped if p.suffix.lower() in GAME_FILE]
    if bad:
        print('build_pages: game files would be published:', *bad, sep='\n  ', file=sys.stderr)
        return 1
    size = sum(p.stat().st_size for p in shipped)
    print(f'build_pages: {SITE} ({len(shipped)} files, {size / 1e6:.1f} MB)')
    return 0


if __name__ == '__main__':
    sys.exit(main())
