/* GTA2 browser port — core ABI contract v1.0 (T04)
 *
 * Binding decisions: D02 (language/dependency boundaries), D03 (runtime and
 * language boundary), D05 (numeric compatibility), D16 (scope boundaries).
 * Contract: docs/contracts/platform-abi-v1.md
 *
 * Rules enforced by this header (checked by docs/contracts/checks/check_contracts.py):
 *   - every exported symbol has C linkage, returns gta2_status unless it is a
 *     pure getter (version / last-error), and uses fixed-width or opaque numeric
 *     handle types only;
 *   - object pointers never cross as JavaScript object identity, network IDs or
 *     serialized bytes; the only pointers crossing the boundary are
 *     (pointer, length) buffers into Wasm linear memory and core-owned
 *     read-only views that die at the next gta2_* call;
 *   - no Windows emulation, injection, DirectPlay, Miles, Bink, MASM, Detours,
 *     GPL loader or D3D path may be declared or enabled here.
 */
#ifndef GTA2_ABI_H
#define GTA2_ABI_H

#include <stdint.h>
#include <stddef.h>

#if defined(GTA2_ENABLE_WINDOWS_EMULATION) || defined(GTA2_ENABLE_PTHREADS) || \
    defined(GTA2_ENABLE_ASYNCIFY) || defined(GTA2_ENABLE_SHARED_ARRAY_BUFFER) || \
    defined(GTA2_ENABLE_WEBGPU) || defined(GTA2_ENABLE_OFFSCREENCANVAS) || \
    defined(GTA2_ENABLE_HOOK_LOADER) || defined(GTA2_ENABLE_DIRECTPLAY) || \
    defined(GTA2_ENABLE_MILES) || defined(GTA2_ENABLE_BINK) || \
    defined(GTA2_ENABLE_D3D)
#error "GTA2 ABI contract: this service is excluded by D02/D03/D16 and must fail at build time, never silently at runtime."
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------ */
/* Contract versions                                                   */
/* ------------------------------------------------------------------ */
/* GTA2_MAKE_VERSION packs major in the high 16 bits and minor in the low
 * 16 bits of a uint32 little-endian field. A major bump breaks ABI or wire
 * compatibility; a minor bump is additive and unknown fields/records are
 * rejected by the receiver rather than guessed (D12).                  */
#define GTA2_MAKE_VERSION(major, minor) \
    ((uint32_t)((((uint32_t)(major)) << 16) | ((uint32_t)(minor))))

#define GTA2_CONTRACT_ABI_VERSION          GTA2_MAKE_VERSION(1u, 0u)
#define GTA2_CONTRACT_PLATFORM_VERSION     GTA2_MAKE_VERSION(1u, 0u)
#define GTA2_CONTRACT_SIMULATION_VERSION   GTA2_MAKE_VERSION(1u, 0u)
#define GTA2_CONTRACT_RENDER_VERSION       GTA2_MAKE_VERSION(1u, 0u)
#define GTA2_CONTRACT_SAVE_VERSION         GTA2_MAKE_VERSION(1u, 0u)
#define GTA2_CONTRACT_NETWORK_VERSION      GTA2_MAKE_VERSION(1u, 0u)
#define GTA2_CONTRACT_PACK_VERSION         GTA2_MAKE_VERSION(1u, 0u)

/* ------------------------------------------------------------------ */
/* Status codes (D03)                                                  */
/* ------------------------------------------------------------------ */
/* Every fallible operation returns int32 and sets the retrievable error
 * string. GTA2_STATUS_UNSUPPORTED is the only permitted answer for an
 * operation or service that is not implemented: returning success while
 * doing nothing, or silently ignoring an unimplemented request, is a
 * contract violation (D02, D03, D16).                                  */
typedef int32_t gta2_status;

enum {
    GTA2_STATUS_OK                  = 0,
    GTA2_STATUS_INVALID_ARGUMENT    = 1,  /* null/invalid buffer, bad flag, bad enum */
    GTA2_STATUS_INVALID_HANDLE      = 2,
    GTA2_STATUS_INVALID_STATE       = 3,  /* op illegal in this mode/phase */
    GTA2_STATUS_UNSUPPORTED         = 4,  /* not implemented / excluded: visible failure */
    GTA2_STATUS_PROFILE_MISMATCH    = 5,
    GTA2_STATUS_ASSET_MISSING       = 6,
    GTA2_STATUS_IO                  = 7,
    GTA2_STATUS_SCHEMA              = 8,  /* bad/unknown schema or record type */
    GTA2_STATUS_CHECKSUM            = 9,
    GTA2_STATUS_OVERFLOW            = 10, /* arithmetic/offset overflow */
    GTA2_STATUS_FATAL_NUMERIC       = 11, /* e.g. fixed-point divide by zero (D05) */
    GTA2_STATUS_OVERLOAD            = 12, /* measured backpressure / catch-up overrun */
    GTA2_STATUS_PROTOCOL            = 13, /* wire violation */
    GTA2_STATUS_DECODE              = 14,
    GTA2_STATUS_CANCELLED           = 15,
    GTA2_STATUS_LIMIT               = 16, /* declared limit exceeded */
    GTA2_STATUS_PAUSED              = 17, /* visible performance pause (D04) */
    GTA2_STATUS_TRUNCATED           = 18  /* bounds-checked read past end */
};

/* ------------------------------------------------------------------ */
/* Handles and buffers (D03)                                           */
/* ------------------------------------------------------------------ */
#define GTA2_SESSION_INVALID 0u
typedef uint32_t gta2_session;

/* v1 runs one Wasm instance on the browser main thread; at most one live
 * session handle exists per instance (D03).                            */
#define GTA2_MAX_LIVE_SESSIONS 1u

#define GTA2_ERROR_MESSAGE_MAX_BYTES 512u

/* Core-owned read-only error view. Valid only until the next gta2_* call;
 * JavaScript must copy immediately and must not retain a stale typed-array
 * view across an allocation that can grow memory (D03).                */
typedef struct gta2_error_info {
    const char* message_utf8; /* never NULL when length > 0; ASCII-safe  */
    uint32_t    length;       /* bytes, excluding any NUL                 */
    gta2_status code;
    uint32_t    reserved0;
} gta2_error_info;

/* ------------------------------------------------------------------ */
/* Required exported operations                                        */
/* ------------------------------------------------------------------ */
/* Pure getters. gta2_last_error_message returns a pointer owned by the
 * core; the pointer and its bytes are invalidated by any later gta2_*
 * call, including another error query.                                */
uint32_t          gta2_contract_version(void);
gta2_error_info   gta2_last_error(void);
const char*       gta2_last_error_message(void);
uint32_t          gta2_last_error_length(void);

/* Fails with GTA2_STATUS_PROFILE_MISMATCH if host_contract_version has a
 * different major, or GTA2_STATUS_UNSUPPORTED if the minor is newer than
 * this build understands.                                              */
gta2_status       gta2_contract_check(uint32_t host_contract_version);

/* Session lifecycle is declared in gta2/platform.h; save framing in
 * gta2/save.h; replication in gta2/net.h; the render view in gta2/render.h.
 * gta2/gta2.h includes all of them.                                    */

/* ------------------------------------------------------------------ */
/* Compile-time ABI shape assertions                                   */
/* ------------------------------------------------------------------ */
/* Field offsets are asserted rather than struct sizes so the same header
 * is valid for wasm32 (32-bit pointers) and for a 64-bit host test build.
 * JavaScript reads the struct with a 4-byte pointer field (D03).       */
#if defined(__cplusplus)
#define GTA2_STATIC_ASSERT(cond, msg) static_assert(cond, msg)
#else
#define GTA2_STATIC_ASSERT(cond, msg) _Static_assert(cond, msg)
#endif

GTA2_STATIC_ASSERT(sizeof(gta2_status) == 4, "gta2_status must be 32-bit");
GTA2_STATIC_ASSERT(sizeof(gta2_session) == 4, "gta2_session must be 32-bit");
GTA2_STATIC_ASSERT(offsetof(gta2_error_info, message_utf8) == 0,
                   "gta2_error_info.message_utf8 must be first");
GTA2_STATIC_ASSERT(offsetof(gta2_error_info, length) == sizeof(const char*),
                   "gta2_error_info.length must follow the pointer");
GTA2_STATIC_ASSERT(offsetof(gta2_error_info, code) == sizeof(const char*) + 4u,
                   "gta2_error_info.code offset is part of the ABI");
GTA2_STATIC_ASSERT(offsetof(gta2_error_info, reserved0) == sizeof(const char*) + 8u,
                   "gta2_error_info.reserved0 offset is part of the ABI");

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* GTA2_ABI_H */
