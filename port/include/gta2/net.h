/* GTA2 browser port — replication wire contract v1 (T04)
 *
 * Binding decisions: D10 (authority and client model), D11 (transport and
 * room service), D12 (inputs, snapshots, bandwidth failure), D14 (joining,
 * disconnection, campaign ownership), D05 (LE fields, no raw structs).
 * Contract: docs/contracts/replication-protocol-v1.md
 *
 * Transport binding, signalling and measured behaviour are owned by T18
 * (docs/contracts/network-v1.md). This header fixes framing, field layout,
 * limits and the authority rules the wire format may not violate.
 * No gameplay WebSocket fallback, peer mesh, distributed simulation, host
 * migration or client physics prediction exists in v1.
 */
#ifndef GTA2_NET_H
#define GTA2_NET_H

#include "gta2/abi.h"
#include "gta2/fixed.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---------------- Channels (D11) ---------------- */
#define GTA2_NET_CHANNEL_CONTROL "control-v1"
#define GTA2_NET_CHANNEL_INPUT   "input-v1"
#define GTA2_NET_CHANNEL_STATE   "state-v1"

/* ordered: 1 = ordered delivery requested; reliability: 1 = reliable.
 * maxRetransmits: 0 means configured to 0; GTA2_NET_UNSET means the property
 * must NOT be set. maxPacketLifeTime must never be set together with
 * maxRetransmits (D11). */
#define GTA2_NET_UNSET (-1)
#define GTA2_NET_CONTROL_ORDERED        1
#define GTA2_NET_CONTROL_RELIABLE       1
#define GTA2_NET_CONTROL_MAX_RETRANSMITS GTA2_NET_UNSET
#define GTA2_NET_CONTROL_MAX_PACKET_LIFE_MS GTA2_NET_UNSET
#define GTA2_NET_INPUT_ORDERED          0
#define GTA2_NET_INPUT_RELIABLE         0
#define GTA2_NET_INPUT_MAX_RETRANSMITS  0
#define GTA2_NET_INPUT_MAX_PACKET_LIFE_MS GTA2_NET_UNSET
#define GTA2_NET_STATE_ORDERED          0
#define GTA2_NET_STATE_RELIABLE         0
#define GTA2_NET_STATE_MAX_RETRANSMITS  0
#define GTA2_NET_STATE_MAX_PACKET_LIFE_MS GTA2_NET_UNSET

#define GTA2_NET_CONTROL_JSON_SMALL_MESSAGES 1u /* JSON for small control messages   */
#define GTA2_NET_INPUT_BINARY_LE             1u /* binary LE for inputs and snapshots */
#define GTA2_NET_WEBSOCKET_GAMEPLAY_FALLBACK 0u /* forbidden                          */
#define GTA2_NET_TOPOLOGY_HOST_STAR          1u /* guests connect to the host only    */
#define GTA2_NET_HOST_MIGRATION              0u /* no election, no migration          */
#define GTA2_NET_CLIENT_PHYSICS_PREDICTION   0u /* forbidden in v1                    */
#define GTA2_NET_ROLLBACK                    0u /* forbidden in v1                    */

#define GTA2_NET_MAX_PLAYERS 4u
#define GTA2_NET_PLAYER_SLOT_MIN 0u
#define GTA2_NET_PLAYER_SLOT_MAX 3u

/* ---------------- Snapshot header (D12), 32 bytes LE ---------------- */
#define GTA2_NET_SNAPSHOT_MAGIC        "G2N1"
#define GTA2_NET_SNAPSHOT_MAGIC_BYTES  4u
#define GTA2_NET_SNAPSHOT_HEADER_BYTES 32u
#define GTA2_NET_PROTOCOL_VERSION      1u

#define GTA2_NET_OFF_MAGIC        0u   /* 4 bytes ASCII 'G','2','N','1'      */
#define GTA2_NET_OFF_PROTOCOL     4u   /* uint16                            */
#define GTA2_NET_OFF_KIND         6u   /* uint16                            */
#define GTA2_NET_OFF_EPOCH        8u   /* uint32, session/campaign epoch     */
#define GTA2_NET_OFF_SEQUENCE     12u  /* uint32, monotonic per connection   */
#define GTA2_NET_OFF_TICK         16u  /* uint32, authoritative tick index   */
#define GTA2_NET_OFF_BASE         20u  /* uint32, baseline sequence (0 none) */
#define GTA2_NET_OFF_PART_INDEX   24u  /* uint16                            */
#define GTA2_NET_OFF_PART_COUNT   26u  /* uint16                            */
#define GTA2_NET_OFF_PAYLOAD_LEN  28u  /* uint32, bytes after the header     */

#define GTA2_NET_KIND_FULL_BASELINE 1u
#define GTA2_NET_KIND_DELTA         2u

/* Sequence policy: 0 means "no baseline"; live sequences start at 1 and never
 * wrap inside a session. */
#define GTA2_NET_SEQUENCE_NO_BASELINE 0u
#define GTA2_NET_SEQUENCE_FIRST_LIVE  1u

/* ---------------- Snapshot payload records (D12) ----------------
 * 16-byte record header, then a typed payload whose schema is explicit per
 * record type. Unknown record types reject the update and request a
 * version-compatible resync; they are never parsed by guesswork. */
#define GTA2_NET_RECORD_HEADER_BYTES 16u
#define GTA2_NET_REC_OFF_OP          0u  /* uint8  */
#define GTA2_NET_REC_OFF_TYPE        1u  /* uint8  */
#define GTA2_NET_REC_OFF_FLAGS       2u  /* uint16 */
#define GTA2_NET_REC_OFF_ENTITY_ID   4u  /* uint32 */
#define GTA2_NET_REC_OFF_GENERATION  8u  /* uint32 */
#define GTA2_NET_REC_OFF_BYTE_LENGTH 12u /* uint32, typed payload bytes */

#define GTA2_NET_OP_UPSERT 0u
#define GTA2_NET_OP_REMOVE 1u
#define GTA2_NET_OP_EVENT  2u
#define GTA2_NET_OP_MAX    GTA2_NET_OP_EVENT

/* Entity identity is always uint32 id plus uint32 generation, assigned on
 * authoritative creation; pointers and array order are never identities
 * (D10). Removal is explicit: an entity is hidden or despawned only on an
 * authoritative removal record, never because a packet was lost (D12). */
#define GTA2_NET_ENTITY_ID_INVALID 0u

/* Typed record types. Static assets are referenced by local profile hashes
 * and are never transferred (D12, D14). */
#define GTA2_NET_TYPE_PLAYER_STATE   1u
#define GTA2_NET_TYPE_PLAYER_HUD     2u
#define GTA2_NET_TYPE_ENTITY_DYNAMIC 3u
#define GTA2_NET_TYPE_WORLD_EDIT     4u
#define GTA2_NET_TYPE_EFFECT         5u
#define GTA2_NET_TYPE_MISSION_EVENT  6u
#define GTA2_NET_TYPE_PROGRESSION    7u

/* ---------------- Critical events (D12) ---------------- */
#define GTA2_NET_EVENT_SEQUENCE_FIRST 1u
#define GTA2_NET_EVENT_DEDUPE_HIGHEST_CONTIGUOUS 1u

/* ---------------- Input frames (D12), 24 bytes LE ---------------- */
#define GTA2_NET_INPUT_FRAME_BYTES   24u
#define GTA2_NET_INPUT_PREVIOUS_FRAMES 2u
#define GTA2_NET_INPUT_MAX_BATCH_BYTES (GTA2_NET_INPUT_FRAME_BYTES * \
                                        (GTA2_NET_INPUT_PREVIOUS_FRAMES + 1u))
#define GTA2_NET_IN_OFF_PROTOCOL     0u  /* uint16, GTA2_NET_PROTOCOL_VERSION */
#define GTA2_NET_IN_OFF_FLAGS        2u  /* uint16, reserved, must be 0       */
#define GTA2_NET_IN_OFF_EPOCH        4u  /* uint32 session epoch              */
#define GTA2_NET_IN_OFF_SLOT         8u  /* uint8 player slot 0..3            */
#define GTA2_NET_IN_OFF_RESERVED0    9u  /* uint8, must be 0                  */
#define GTA2_NET_IN_OFF_BUTTONS      10u /* uint16 bitset                     */
#define GTA2_NET_IN_OFF_SEQUENCE     12u /* uint32 monotonic per slot         */
#define GTA2_NET_IN_OFF_CLIENT_TICK  16u /* uint32 client-estimated host tick */
#define GTA2_NET_IN_OFF_AIM_DX       20u /* int16 Q15                         */
#define GTA2_NET_IN_OFF_AIM_DY       22u /* int16 Q15                         */

/* Batch order is oldest first, newest last; the latest frame is authoritative
 * and earlier frames exist only for loss resilience. */
#define GTA2_NET_INPUT_BATCH_OLDEST_FIRST 1u
#define GTA2_NET_NEUTRALIZE_INPUT_AFTER_MS 250u
#define GTA2_NET_INPUT_RESEND_MS           33u
#define GTA2_NET_NO_ARTIFICIAL_INPUT_DELAY 1u /* no rollback, no two-tick delay */
#define GTA2_NET_EDGE_ACTION_DEDUPE_BY_SEQUENCE 1u
#define GTA2_NET_REJECT_FUTURE_TICK_CLAIMS  1u

/* ---------------- Cadence and limits (D04, D12) ---------------- */
#define GTA2_NET_SNAPSHOT_MS                  33u
#define GTA2_NET_INTERPOLATION_TICKS          2u   /* render 66 ms behind            */
#define GTA2_NET_MAX_VISUAL_EXTRAPOLATION_TICKS 1u /* then freeze until fresh state  */
#define GTA2_NET_SNAPSHOT_HISTORY_COUNT       64u
#define GTA2_NET_MAX_PAYLOAD_BYTES            12288u  /* 12 KiB per message         */
#define GTA2_NET_MAX_SNAPSHOT_PARTS           512u
#define GTA2_NET_MAX_REASSEMBLED_BYTES        4194304u /* 4 MiB                     */
#define GTA2_NET_INCOMPLETE_DELTA_TIMEOUT_MS  200u
#define GTA2_NET_FULL_BASELINE_TIMEOUT_MS     10000u
#define GTA2_NET_MAX_INCOMPLETE_DELTAS        2u
#define GTA2_NET_MAX_INCOMPLETE_FULL_BASELINES 1u
#define GTA2_NET_STATE_BACKPRESSURE_BYTES     262144u  /* 256 KiB                   */
#define GTA2_NET_CONTROL_BACKPRESSURE_BYTES   1048576u /* 1 MiB                     */
#define GTA2_NET_CONGESTION_TIMEOUT_MS        10000u
#define GTA2_NET_STEADY_UPLOAD_BYTES_PER_SECOND_PER_GUEST 1048576u /* 1 MiB/s     */
#define GTA2_NET_ACK_AT_MOST_ONCE_PER_TICK    1u
#define GTA2_NET_DELTA_REQUIRES_BASELINE      1u /* apply atomically or not at all */

/* ---------------- Room and session limits (D11, D14) ---------------- */
#define GTA2_ROOM_JOIN_TOKEN_BITS            128u
#define GTA2_ROOM_MAX_SIGNALING_MESSAGE_BYTES 65536u
#define GTA2_ROOM_SIGNALING_MESSAGES_PER_SECOND 30u
#define GTA2_ROOM_UNUSED_EXPIRY_MS           60000u
#define GTA2_ROOM_RECONNECT_GRACE_MS         60000u
#define GTA2_ROOM_NON_ACTOR_QUORUM_REMOVAL_MS 2000u
#define GTA2_ROOM_HOST_HEARTBEAT_TIMEOUT_MS  5000u
#define GTA2_ROOM_JOIN_BETWEEN_MISSIONS_ONLY 1u
#define GTA2_ROOM_GUEST_IMPORTS_OWN_ASSETS   1u /* host never transfers assets */
#define GTA2_ROOM_SAVE_OWNER_IS_HOST         1u
#define GTA2_ROOM_INVITE_TOKEN_IN_URL_FRAGMENT 1u
#define GTA2_ROOM_STORE_ASSETS_ON_SIGNALING  0u /* forbidden */

/* ---------------- Encoding and applying views ---------------- */
/* Options handed to gta2_net_encode_view. Fields are LE uint32, read by the
 * core with the bounds-checked reader, never by struct layout. */
#define GTA2_NET_ENCODE_OPTS_BYTES 16u
#define GTA2_NET_OPT_OFF_KIND         0u /* uint32: GTA2_NET_KIND_*               */
#define GTA2_NET_OPT_OFF_BASE         4u /* uint32: acknowledged base sequence    */
#define GTA2_NET_OPT_OFF_MAX_PAYLOAD  8u /* uint32: <= GTA2_NET_MAX_PAYLOAD_BYTES */
#define GTA2_NET_OPT_OFF_PART_INDEX   12u/* uint32: request this part, or UINT32_MAX */

gta2_status gta2_net_encode_view(gta2_session session, const void* opts_bytes,
                                 uint32_t opts_len, uint8_t* out, uint32_t capacity,
                                 uint32_t* out_written, uint32_t* out_required);
gta2_status gta2_net_apply_view(gta2_session session, const uint8_t* in, uint32_t length);
gta2_status gta2_net_ack_applied(gta2_session session, uint32_t sequence);

#if defined(__cplusplus)
#define GTA2_NET_ASSERT(cond, msg) static_assert(cond, msg)
#else
#define GTA2_NET_ASSERT(cond, msg) _Static_assert(cond, msg)
#endif

GTA2_NET_ASSERT(GTA2_NET_SNAPSHOT_HEADER_BYTES == 32u, "snapshot header is 32 bytes");
GTA2_NET_ASSERT(GTA2_NET_OFF_PAYLOAD_LEN + 4u == GTA2_NET_SNAPSHOT_HEADER_BYTES,
                "snapshot header field offsets must total 32 bytes");
GTA2_NET_ASSERT(GTA2_NET_INPUT_MAX_BATCH_BYTES == 72u, "input batch is at most 3 frames");
GTA2_NET_ASSERT(GTA2_NET_IN_OFF_AIM_DY + 2u == GTA2_NET_INPUT_FRAME_BYTES,
                "input frame field offsets must total 24 bytes");

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* GTA2_NET_H */
