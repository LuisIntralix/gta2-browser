/* GTA2 browser port — umbrella contract header v1.0 (T04)
 *
 * This file is the complete public interface of the Wasm core. Anything not
 * declared here is an implementation detail and must not be exported.
 *
 * Boundary doctrine (D02, D03, D16):
 *   - one Wasm instance, main thread, no pthreads/SharedArrayBuffer/Asyncify/
 *     WebGPU/OffscreenCanvas; the Emscripten ES module factory is the only
 *     entry point;
 *   - extern "C" only; numeric handles plus (pointer, length) buffers;
 *   - every fallible operation returns gta2_status and sets a retrievable
 *     error string; an unimplemented or excluded service returns
 *     GTA2_STATUS_UNSUPPORTED or a more specific error, never success;
 *   - no raw pointer, C++ object or host struct layout is ever serialized;
 *     all multi-byte serialization is explicit little-endian;
 *   - core-owned views (error string, render view, event buffer) die at the
 *     next gta2_* call, because linear memory can grow (ALLOW_MEMORY_GROWTH);
 *     JavaScript reacquires typed-array views after every call.
 *
 * Excluded components (recorded in docs/decisions/permissions.md) are not
 * declared, linked or emulated here: the Windows hook/injection path, the
 * original executable, DirectPlay, the Miles and Bink SDKs and DLLs, the VC98
 * toolchain and MASM path, the GTA2Hax D3D9 renderer and Microsoft D3D/
 * DirectDraw headers, and any UI framework. The build-time guards in
 * gta2/abi.h turn an attempt to enable one of them into a compile error.
 */
#ifndef GTA2_GTA2_H
#define GTA2_GTA2_H

#include "gta2/abi.h"
#include "gta2/fixed.h"
#include "gta2/platform.h"
#include "gta2/render.h"
#include "gta2/save.h"
#include "gta2/net.h"
#include "gta2/pack.h"

#endif /* GTA2_GTA2_H */
