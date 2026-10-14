#!/bin/sh
# Drive the original 9.6F GTA2 under the project-local Wine prefix.
#
#   refcap.sh launch                 start the game, retrying startup crashes
#   refcap.sh input <step>...        inject keys (see gta2_input.c for steps)
#   refcap.sh shot <name>            capture the GTA2 window to $REFCAP_OUT/<name>.png
#   refcap.sh stop                   kill the Wine session
#
# REFCAP_EXE picks the program: GTA2\gta2.exe (original, default) or
# GTA2SA\decomp_main.exe (upstream standalone reconstruction).
# Prefix setup (game copy, registry) is described in docs/reference-runtime.md.
set -eu
root=$(cd "$(dirname "$0")/../.." && pwd)
export WINEPREFIX="$root/research/wineprefix/gta2"
export WINEDEBUG="${WINEDEBUG:-err+seh,-d3d}"
exe="${REFCAP_EXE:-GTA2\\gta2.exe}"
wine_bin="$root/research/toolchains/wine/Wine Devel.app/Contents/Resources/wine/bin"
out="${REFCAP_OUT:-$root/research/runtime-data/refcap}"
mkdir -p "$out"

window_id() {
    swift - <<'EOF' 2>/dev/null
import CoreGraphics
let l = CGWindowListCopyWindowInfo([.optionOnScreenOnly], kCGNullWindowID) as! [[String: Any]]
for w in l where (w["kCGWindowName"] as? String) == "GTA2" { print(w["kCGWindowNumber"]!); break }
EOF
}

cmd="${1:-}"
[ $# -gt 0 ] && shift
case "$cmd" in
launch)
    # Rosetta 2 intermittently runs Wine's 64-bit syscall thunk in 32-bit mode
    # during startup (fault reading 0x2DC1 in wow64cpu). Detect it and relaunch.
    dir="$WINEPREFIX/drive_c/${exe%\\*}"
    for try in 1 2 3 4 5 6 7 8; do
        "$wine_bin/wineserver" -k 2>/dev/null || true
        sleep 1
        (cd "$dir" && "$wine_bin/wine" "${exe##*\\}" >"$out/gta2.log" 2>&1 &)
        sleep 14
        if ! grep -qE 'Unhandled page fault|unable to dispatch|nested exception' "$out/gta2.log" &&
            [ -n "$(window_id)" ]; then
            echo "started on try $try"
            exit 0
        fi
    done
    echo "startup failed after $try tries; see $out/gta2.log" >&2
    exit 1
    ;;
input)
    "$wine_bin/wine" 'C:\tools\gta2_input.exe' GTA2 "$@" >/dev/null 2>&1
    ;;
shot)
    id=$(window_id)
    [ -n "$id" ] || { echo "GTA2 window not on screen" >&2; exit 1; }
    screencapture -x -l "$id" "$out/$1.png"
    echo "$out/$1.png"
    ;;
stop)
    "$wine_bin/wineserver" -k
    ;;
*)
    echo "usage: $0 launch|input|shot|stop" >&2
    exit 2
    ;;
esac
