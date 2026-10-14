/* GTA2 browser port — native browser save schema v1 (T04)
 *
 * Binding decisions: D09 (saves and recovery), D06 (framing style reused),
 * D10 (campaign ownership), D05 (LE readers).
 * Contract: docs/contracts/save-schema-v1.md
 *
 * A save is logical state only. Raw Wasm memory, C++ object bytes and
 * pointer values are never serialized; references are stable uint32 IDs plus
 * generation. JavaScript owns IndexedDB; the core only encodes and decodes
 * the framed bytes.
 */
#ifndef GTA2_SAVE_H
#define GTA2_SAVE_H

#include "gta2/abi.h"
#include "gta2/fixed.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GTA2_SAVE_MAGIC              "GTA2SV01" /* 8 ASCII bytes, no NUL in file */
#define GTA2_SAVE_MAGIC_BYTES        8u
#define GTA2_SAVE_SCHEMA_VERSION     1u

/* Same simple framed layout as the pack, with the save magic (D09):
 * magic[8] || metadata_bytes:u32 || entry_count:u32 || UTF-8 JSON metadata
 * || zero padding to 8 bytes || concatenated state payloads. */
#define GTA2_SAVE_HEADER_BYTES       16u
#define GTA2_SAVE_OFF_MAGIC          0u
#define GTA2_SAVE_OFF_METADATA_BYTES 8u
#define GTA2_SAVE_OFF_ENTRY_COUNT    12u
#define GTA2_SAVE_ALIGNMENT          8u
#define GTA2_SAVE_HASH_SHA256_BYTES  32u

/* IndexedDB layout (D09). Database version and store names are fixed. */
#define GTA2_SAVE_DB_NAME            "gta2-browser-v1"
#define GTA2_SAVE_DB_VERSION         1u
#define GTA2_SAVE_STORE_REVISIONS    "save-revisions"
#define GTA2_SAVE_STORE_SLOT_HEADS   "save-heads"
#define GTA2_SAVE_STORE_ASSET_CACHE  "asset-cache"
#define GTA2_SAVE_STORE_MEDIA        "converted-media"
#define GTA2_SAVE_STORE_PREFERENCES  "preferences"

#define GTA2_SAVE_NAMED_SLOTS_PER_CAMPAIGN   10u
#define GTA2_SAVE_RETAINED_REVISIONS_PER_SLOT 2u
#define GTA2_SAVE_REVISION_COMMIT_TRANSACTION 1u /* revision + checksum + head, one transaction */

#define GTA2_SAVE_MODE_SP            1u
#define GTA2_SAVE_MODE_COOP          2u

/* Explicit scope decisions: not features to be added later by a worker. */
#define GTA2_SAVE_RAW_MEMORY_DUMP          0u /* forbidden                       */
#define GTA2_SAVE_ORIGINAL_SVG_INTEROP     0u /* out of v1 scope; keep originals */
#define GTA2_SAVE_COOP_TO_SP               0u /* conversion absent               */
#define GTA2_SAVE_SP_TO_COOP_MODE          1u /* clone between missions only     */
#define GTA2_SAVE_AUTOSAVE_BETWEEN_MISSIONS_OR_DISTRICTS 1u
#define GTA2_SAVE_CAMPAIGN_OWNER_IS_HOST   1u

/* Metadata JSON fields written by gta2_save_encode and required on decode. */
#define GTA2_SAVE_FIELD_SCHEMA       "schema_version"
#define GTA2_SAVE_FIELD_PROFILE      "profile_hash"
#define GTA2_SAVE_FIELD_MODE         "mode"
#define GTA2_SAVE_FIELD_CAMPAIGN     "campaign_uuid"
#define GTA2_SAVE_FIELD_SLOT         "slot_id"
#define GTA2_SAVE_FIELD_REVISION     "revision"
#define GTA2_SAVE_FIELD_TICK         "tick"
#define GTA2_SAVE_FIELD_CREATED_UTC  "created_utc"
#define GTA2_SAVE_FIELD_BUILD        "build_id"
#define GTA2_SAVE_FIELD_ENTRIES      "entries"

/* Snapshot sections a v1 save must contain. Each section is a named payload
 * with its own schema_version and SHA-256 in the metadata. Fields are
 * enumerated mechanically from source by T11; a field whose semantics are
 * unresolved becomes a specialist ticket, never a guessed encoding. */
#define GTA2_SAVE_SECTION_ENTITIES   "entities"
#define GTA2_SAVE_SECTION_SCRIPTS    "script-threads"
#define GTA2_SAVE_SECTION_PLAYERS    "players"
#define GTA2_SAVE_SECTION_TIMERS     "timers"
#define GTA2_SAVE_SECTION_RNG        "rng"
#define GTA2_SAVE_SECTION_PROGRESS   "progression"
#define GTA2_SAVE_SECTION_WORLD      "world-modifications"

/* Encode the current session as an export. Preconditions (D09):
 *   - the session is at a completed authoritative tick and no tick is in
 *     progress; nothing mutates authoritative state until the call returns;
 *   - the profile hash matches the mounted profile;
 *   - two-phase sizing: call with out == NULL / capacity == 0 to receive
 *     out_required, then call again with capacity >= required, otherwise
 *     GTA2_STATUS_LIMIT is returned and nothing is written.
 * Postcondition: out is exactly the framed export defined above, or an error
 * status is returned with no partial frame committed. */
gta2_status gta2_save_encode(gta2_session session, uint8_t* out, uint32_t capacity,
                             uint32_t* out_written, uint32_t* out_required);

/* Decode and apply an export. Rejects magic/schema/profile mismatch with
 * GTA2_STATUS_SCHEMA, GTA2_STATUS_PROFILE_MISMATCH or GTA2_STATUS_CHECKSUM
 * before mutating any state. A corrupt or newer save is never partially
 * applied; the caller preserves it for export and offers the previous
 * revision (D09). */
gta2_status gta2_save_decode(gta2_session session, const uint8_t* in, uint32_t length);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* GTA2_SAVE_H */
