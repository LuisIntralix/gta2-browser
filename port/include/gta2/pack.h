/* GTA2 browser port — .gta2pack v1 framing contract (T04)
 *
 * Binding decisions: D06 (asset import and memory), D05 (LE readers,
 * uint32 file offsets), D16 (no bundled assets).
 * Contract: docs/contracts/pack-format-v1.md
 *
 * The pack is produced and validated outside Wasm (JS importer, T07) and
 * installed into Emscripten MEMFS by JavaScript. v1 defines no core-side
 * pack reader: the core receives already-validated manifest JSON plus MEMFS
 * files, so no pack parsing is hidden behind a C stub.
 */
#ifndef GTA2_PACK_H
#define GTA2_PACK_H

#include "gta2/abi.h"
#include "gta2/fixed.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GTA2_PACK_MAGIC              "GTA2PK01" /* 8 ASCII bytes, no NUL in file */
#define GTA2_PACK_MAGIC_BYTES        8u
#define GTA2_PACK_SCHEMA_VERSION     1u

/* Header (16 bytes total): magic[8] || manifest_bytes:u32 || entry_count:u32 */
#define GTA2_PACK_HEADER_BYTES       16u
#define GTA2_PACK_OFF_MAGIC          0u
#define GTA2_PACK_OFF_MANIFEST_BYTES 8u
#define GTA2_PACK_OFF_ENTRY_COUNT    12u

/* UTF-8 JSON manifest, then zero padding to an 8-byte boundary, then the
 * concatenated payloads. Payload offsets are relative to the start of the
 * payload area and are validated with the bounds-checked reader.          */
#define GTA2_PACK_PAYLOAD_ALIGNMENT  8u
#define GTA2_PACK_MANIFEST_ENCODING_UTF8 1u
#define GTA2_PACK_COMPRESSION_NONE   0u
#define GTA2_PACK_HASH_SHA256_BYTES  32u

/* Limits (D06). A file exceeding a limit is reported to the orchestrator
 * with its size; it is never silently dropped and the limit is never lifted
 * by a worker. */
#define GTA2_PACK_MAX_TOTAL_BYTES    2147483648u /* 2 GiB  */
#define GTA2_PACK_MAX_ENTRY_BYTES    536870912u  /* 512 MiB */
#define GTA2_PACK_MAX_ENTRIES        4096u
#define GTA2_PACK_MAX_MANIFEST_BYTES 1048576u    /* 1 MiB */

/* Required manifest fields per entry, and the profile fields. Field names
 * are the JSON keys the importer writes and the core accepts. */
#define GTA2_PACK_MANIFEST_FIELD_PATH      "path"
#define GTA2_PACK_MANIFEST_FIELD_ROLE      "role"
#define GTA2_PACK_MANIFEST_FIELD_OFFSET    "offset"
#define GTA2_PACK_MANIFEST_FIELD_LENGTH    "length"
#define GTA2_PACK_MANIFEST_FIELD_SHA256    "sha256"
#define GTA2_PACK_MANIFEST_FIELD_PROFILE   "profile_hash"
#define GTA2_PACK_MANIFEST_FIELD_SCHEMA    "schema_version"
#define GTA2_PACK_MANIFEST_FIELD_SOURCE    "conversion_provenance"

/* Roles a manifest entry may declare. Core-required roles for a district
 * must be mounted in MEMFS before that district's tick loop starts. */
#define GTA2_PACK_ROLE_MAP        "map"     /* GMP */
#define GTA2_PACK_ROLE_STYLE      "style"   /* STY */
#define GTA2_PACK_ROLE_SCRIPT     "script"  /* MIS/compiled */
#define GTA2_PACK_ROLE_TEXT       "text"
#define GTA2_PACK_ROLE_SFX        "sfx"
#define GTA2_PACK_ROLE_MUSIC      "music"
#define GTA2_PACK_ROLE_VIDEO      "video"
#define GTA2_PACK_ROLE_OTHER      "other"

/* Rejection rules the importer must enforce before install (D06): payload
 * overlap, duplicate or case-colliding normalized paths, absolute or
 * parent-relative paths, unaligned or overflowing offsets, hash mismatch,
 * manifest/entry limit breach, and unknown schema_version. */
enum {
    GTA2_PACK_REJECT_OK              = 0,
    GTA2_PACK_REJECT_MAGIC           = 1,
    GTA2_PACK_REJECT_SCHEMA          = 2,
    GTA2_PACK_REJECT_MANIFEST_LIMIT  = 3,
    GTA2_PACK_REJECT_ENTRY_LIMIT     = 4,
    GTA2_PACK_REJECT_OVERLAP         = 5,
    GTA2_PACK_REJECT_DUPLICATE_PATH  = 6,
    GTA2_PACK_REJECT_UNSAFE_PATH     = 7,
    GTA2_PACK_REJECT_OFFSET_RANGE    = 8,
    GTA2_PACK_REJECT_ENTRY_LIMIT_BYTES = 9,
    GTA2_PACK_REJECT_TOTAL_LIMIT     = 10,
    GTA2_PACK_REJECT_HASH            = 11,
    GTA2_PACK_REJECT_JSON            = 12
};

GTA2_FIXED_ASSERT(GTA2_PACK_MAGIC_BYTES == 8u, "pack magic is 8 bytes");
GTA2_FIXED_ASSERT(GTA2_PACK_OFF_ENTRY_COUNT + 4u == GTA2_PACK_HEADER_BYTES,
                  "pack header is magic + u32 + u32");

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* GTA2_PACK_H */
