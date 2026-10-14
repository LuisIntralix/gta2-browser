#!/bin/sh
# Builds gta2_memdump.exe (32-bit Windows) like build_gta2_input.sh: Apple clang + emsdk lld, no SDK.
set -eu
here=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$here/../.." && pwd)
llvm="$root/research/toolchains/emsdk/upstream/bin"
out="$root/research/build/refcap"
mkdir -p "$out"
printf 'LIBRARY user32.dll\nEXPORTS\n_FindWindowA@8 == FindWindowA\n_GetWindowThreadProcessId@8 == GetWindowThreadProcessId\n' > "$out/memdump_user32.def"
printf 'LIBRARY kernel32.dll\nEXPORTS\n_OpenProcess@12 == OpenProcess\n_ReadProcessMemory@20 == ReadProcessMemory\n_VirtualQueryEx@16 == VirtualQueryEx\n_CloseHandle@4 == CloseHandle\n' > "$out/memdump_kernel32.def"
printf 'LIBRARY msvcrt.dll\nEXPORTS\nprintf\nfopen\nfwrite\nfclose\nstrtoul\nexit\n__getmainargs\n' > "$out/memdump_msvcrt.def"
for lib in user32 kernel32 msvcrt; do
    "$llvm/lld" -flavor link /lib /machine:x86 /def:"$out/memdump_$lib.def" /out:"$out/memdump_$lib.lib" >/dev/null
done
/usr/bin/clang --target=i686-pc-windows-msvc -O2 -ffreestanding -fno-builtin -fms-extensions \
    -c "$here/gta2_memdump.c" -o "$out/gta2_memdump.obj"
"$llvm/lld" -flavor link /nologo /machine:x86 /subsystem:console /entry:mainCRTStartup /nodefaultlib \
    "$out/gta2_memdump.obj" "$out/memdump_user32.lib" "$out/memdump_kernel32.lib" "$out/memdump_msvcrt.lib" \
    /out:"$out/gta2_memdump.exe"
echo "$out/gta2_memdump.exe"
