# Reference runtime: original GTA2 9.6F under Wine on this Mac

Established 13 October 2026. This is the D01 "original gameplay oracle". It
replaces the Windows 11 host that D01 assumed. The user approved building a
quick Windows environment, and Wine was the fastest route on this Apple
Silicon Mac. Everything lives in ignored `research/`. Nothing is installed
system-wide.

## What works (observed)

| Check | Result |
|---|---|
| Installer extraction | 344 files, 403,201,919 bytes, into `game/`. Installer MD5 unchanged afterwards. |
| `game/gta2.exe` identity | SHA-256 `420af867…e089f`, **identical to the `9.6f.exe` that `gta2_re`'s reccmp config tracks**. |
| Data versions vs reconstructed loaders | STY `GBST` v700 (loader verifies 700) ✓. GXT `GBLE` v100 (loader verifies `GBL`+lang, 100) ✓. GMP `GBMP` v500 (no version check). |
| Original game boots | Main menu "GTA2 V9.6", attract demo, district 1 start screen. |
| Scripted input | Menu navigation, start play, walking. The player walked into the church save point, which triggered "TUTORIAL!" and the script message about the $50,000 save cost. |
| Upstream standalone (`decomp_main.exe`, CI artifact for pin `02de8be`) with the 9.6F renderer DLLs | Boots to the main menu, renders the frontend and accepts menu input. **Entering gameplay is unconfirmed**: the window hides itself and shows a stale frame (see Known issues). |

Evidence screenshots are in `research/runtime-data/refcap/` (local only, since
they contain game imagery).

## Components

| Component | Location | Provenance |
|---|---|---|
| unshield 1.6.2 | `research/toolchains/unshield` (commit `0d1e7cb`, local CMake build) | github.com/twogood/unshield, MIT |
| Wine Devel 11.16 (macOS, wow64 under Rosetta 2) | `research/toolchains/wine/Wine Devel.app` | Gcenx/macOS_Wine_builds, SHA-256 `6f9af818…033cd`, matching Homebrew's `wine@devel` cask |
| Wine prefix | `research/wineprefix/gta2` (`C:\GTA2` original, `C:\GTA2SA` standalone, `C:\tools`) | created locally |
| Input injector | `scripts/refcap/gta2_input.c` → `research/build/refcap/gta2_input.exe` | ours. Apple clang `--target=i686-pc-windows-msvc` + emsdk `lld`, no Windows SDK |
| ImGui stub | `scripts/refcap/imgui_stub.{c,def}` → `dear_imgui.dll` | ours. No-op exports so `decomp_main.exe` loads |
| Upstream standalone exe | `research/upstream-ci/02de8be/` | CI run 38216592513 artifacts (`windows_vc6_main_exe`, `Windows_VC_Latest_main_exe`). **Artifacts expire 2026-10-19** |

## Reproduce

```sh
scripts/extract/extract_game.sh                 # game/ + hashed manifest
scripts/refcap/build_gta2_input.sh              # injector
scripts/refcap/refcap.sh launch                 # original; REFCAP_EXE='GTA2SA\decomp_main.exe' for upstream
scripts/refcap/refcap.sh input tap:RETURN wait:1200 tap:RETURN wait:12000 down:UP wait:1500 up:UP
scripts/refcap/refcap.sh shot after-walk
scripts/refcap/refcap.sh stop
```

Prefix setup, done once:

- copy `game/` to `drive_c/GTA2`, and `gta2_input.exe` to `drive_c/tools`
- `HKLM\Software\DMA Design Ltd\GTA2\Screen`: `start_mode=0` (windowed),
  `do_play_movie=0`
- `HKLM\Software\DMA Design Ltd\GTA2\Debug`: `skip_audio=1`,
  `skip_window_check=1` (all keys written with `/reg:32`)
- `HKLM\Software\Microsoft\Windows NT\CurrentVersion\AeDebug`: `Auto=1`, so
  crashes don't leave dialogs

## Known issues

1. **Startup crash, about 25–50% of launches (Rosetta 2 + Wine wow64).**
   - The fault is a read of `0x2DC1` at `wow64cpu+0x1239`. That instruction
     is `mov 0x2dc1(%rip),%edx` in the 32→64 syscall thunk. A fault at the
     *displacement* means the CPU executed 64-bit code in 32-bit mode, so the
     far-call mode switch failed.
   - The same signature appears with the original exe and with both upstream
     builds, under Wine 11.16 and Wine Staging 11.18.
   - It isn't caused by game code. `refcap.sh launch` detects it and retries.
     It hasn't been seen once a session is running.
2. **No audio.**
   - With Wine's CoreAudio driver enabled and audio not skipped, the Miles
     path made startup crashes more frequent.
   - With the driver disabled, a real game start hung.
   - `skip_audio=1` is the working configuration. Audio parity needs another
     route, for example a Windows VM.
3. **The Vulkan backend (`WINE_D3D_CONFIG=renderer=vulkan`) doesn't work.**
   It fails in buffer creation under MoltenVK. Use the default OpenGL backend.
4. **The upstream standalone pauses when its window is inactive.**
   - In the reconstruction, `skip_window_check` is read (`debug.cpp:156`) but
     never consumed. The window minimises and releases input on deactivation
     (`winmain.cpp:2074`, `:2080`).
   - The macOS Wine driver only treats the window as active when Wine is the
     frontmost macOS app. Activating it with `NSRunningApplication.activate`
     still didn't show gameplay.
   - It's unresolved whether this is focus handling, a 9.6F-DLL/10.5-exe
     mismatch, or a real standalone defect. The DLL *export names* are
     identical: 41 `gbh_*` + 21 `Vid_*`, plus GTA2Hax's extra
     `gbh_SetBeginSceneCB`.
5. **Possible reconstruction difference** (observed, unverified): on the
   "Start play in area" screen, the standalone draws the "1" badge offset from
   its red circle. The original centres it.
