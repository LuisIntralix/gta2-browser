#!/bin/sh
# Builds gta2_input.exe (32-bit Windows) with Apple clang + emsdk lld, no Windows SDK.
# Import libraries are generated from minimal .def files.
set -eu
here=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$here/../.." && pwd)
llvm="$root/research/toolchains/emsdk/upstream/bin"
out="$root/research/build/refcap"
mkdir -p "$out"

printf 'LIBRARY user32.dll\nEXPORTS\n_SendInput@12 == SendInput\n_MapVirtualKeyA@8 == MapVirtualKeyA\n_FindWindowA@8 == FindWindowA\n_SetForegroundWindow@4 == SetForegroundWindow\n_ShowWindow@8 == ShowWindow\n' > "$out/user32.def"
printf 'LIBRARY kernel32.dll\nEXPORTS\n_Sleep@4 == Sleep\n' > "$out/kernel32.def"
printf 'LIBRARY msvcrt.dll\nEXPORTS\nprintf\nfprintf\nfflush\n__iob_func\nstrtoul\nstrncpy\nstrchr\nstrcmp\n_stricmp\ntoupper\nexit\n__getmainargs\n' > "$out/msvcrt.def"
for lib in user32 kernel32 msvcrt; do
    "$llvm/lld" -flavor link /lib /machine:x86 /def:"$out/$lib.def" /out:"$out/$lib.lib" >/dev/null
done

/usr/bin/clang --target=i686-pc-windows-msvc -O2 -ffreestanding -fno-builtin -fms-extensions \
    -c "$here/gta2_input.c" -o "$out/gta2_input.obj"
"$llvm/lld" -flavor link /nologo /machine:x86 /subsystem:console /entry:mainCRTStartup /nodefaultlib \
    "$out/gta2_input.obj" "$out/user32.lib" "$out/kernel32.lib" "$out/msvcrt.lib" /out:"$out/gta2_input.exe"
echo "$out/gta2_input.exe"
