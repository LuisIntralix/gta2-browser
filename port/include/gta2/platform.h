/* GTA2 browser port — platform boundary contract v1 (T04)
 *
 * Binding decisions: D03 (session and boundary operations), D04 (cadence,
 * catch-up cap and visible pause), D06 (profile mount and MEMFS), D08 (audio
 * and video events), D10 (session modes and authority), D14 (pause/reconnect).
 * Contract: docs/contracts/platform-abi-v1.md, docs/contracts/simulation-tick-v1.md
 *
 * JavaScript owns requestAnimationFrame, browser APIs, IndexedDB, Blob URLs
 * and all asynchronous I/O. The core owns authoritative simulation and emits
 * events. A game tick contains no promise wait. No operation is a silent
 * no-op: an unavailable or excluded service returns a status code and an
 * event, never GTA2_STATUS_OK.
 */
#ifndef GTA2_PLATFORM_H
#define GTA2_PLATFORM_H

#include "gta2/abi.h"
#include "gta2/fixed.h"
#include "gta2/render.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---------------- Session modes (D10) ---------------- */
#define GTA2_SESSION_MODE_SP           1u /* single player, authoritative            */
#define GTA2_SESSION_MODE_HOST         2u /* co-op host, authoritative               */
#define GTA2_SESSION_MODE_PRESENTATION 3u /* guest: applies authoritative views only */

/* gta2_session_create descriptor. All fields are little-endian uint32; the
 * core copies it field by field with the bounds-checked reader and never
 * casts the pointer to a struct. desc_len must equal
 * GTA2_SESSION_DESC_BYTES and struct_bytes must match the core's layout. */
#define GTA2_SESSION_DESC_BYTES 32u
#define GTA2_SESSION_DESC_FIELD_STRUCT_BYTES      0u
#define GTA2_SESSION_DESC_FIELD_MODE              4u
#define GTA2_SESSION_DESC_FIELD_PLAYER_SLOT       8u
#define GTA2_SESSION_DESC_FIELD_FLAGS             12u
#define GTA2_SESSION_DESC_FIELD_MAX_TICKS_PER_FRAME 16u
#define GTA2_SESSION_DESC_FIELD_RESERVED0         20u
#define GTA2_SESSION_DESC_FIELD_RESERVED1         24u
#define GTA2_SESSION_DESC_FIELD_RESERVED2         28u

/* Player slots are stable 0..3 (D10). SP always uses slot 0. */
#define GTA2_SESSION_FLAG_NONE 0u

gta2_status gta2_session_create(const void* desc_bytes, uint32_t desc_len,
                                gta2_session* out_session);
gta2_status gta2_session_destroy(gta2_session session);

/* ---------------- Profile and district (D06) ---------------- */
#define GTA2_PROFILE_HASH_SHA256_BYTES 32u

/* Mount an already-validated profile manifest. JavaScript has validated the
 * pack, hashed it and placed the current district's engine-required files in
 * MEMFS; the core only reads them. A mismatched hash, unknown schema or
 * missing required role fails with GTA2_STATUS_PROFILE_MISMATCH,
 * GTA2_STATUS_SCHEMA or GTA2_STATUS_ASSET_MISSING. */
gta2_status gta2_profile_mount(gta2_session session,
                               const uint8_t* manifest_json_utf8, uint32_t manifest_len,
                               const uint8_t* profile_sha256, uint32_t hash_len);

/* Begin/end one district. District files are mounted by JavaScript before
 * gta2_district_begin and unmounted after gta2_district_end; the core never
 * copies the whole installation into memory (D06). */
gta2_status gta2_district_begin(gta2_session session, uint32_t district_index);
gta2_status gta2_district_end(gta2_session session);

/* ---------------- Cadence and tick reference policy (D04) ---------------- */
#define GTA2_SIM_TICK_MS                33u  /* fixed normal tick                 */
#define GTA2_SIM_MAX_CATCHUP_TICKS      4u   /* at most four ticks per callback   */
#define GTA2_SIM_MAX_FRAME_ELAPSED_MS   250u /* elapsed_ms clamp before accrual   */
#define GTA2_SIM_ALPHA_SCALE            65536u
#define GTA2_SIM_FAST_MODE              0u   /* 11 ms fast mode disabled          */
#define GTA2_SIM_LEGACY_MULTIPLAYER_SPEED 0u /* legacy speed options disabled     */
#define GTA2_SIM_SCRIPT_TIMER_30_PER_SECOND 1u /* original 30 * seconds conversion */
#define GTA2_SIM_RNG_ADVANCES_ONLY_ON_TICK 1u  /* tick index and RNG advance only
                                                * when a game update runs           */

/* Pause reasons. A pause is always visible: gta2_frame_pump reports
 * GTA2_STATUS_PAUSED and no tick advances while paused. */
#define GTA2_PAUSE_NONE                  0u
#define GTA2_PAUSE_PERFORMANCE_OVERRUN   1u /* > 4 ticks overdue (D04)            */
#define GTA2_PAUSE_SP_FOCUS_LOSS         2u /* SP only: window/tab loses focus    */
#define GTA2_PAUSE_PAGE_HIDE             3u /* host page hide pauses the room     */
#define GTA2_PAUSE_CUTSCENE_VIDEO        4u /* full-screen story video (D13)      */
#define GTA2_PAUSE_ACTOR_RECONNECT       5u /* actor disconnect grace (D14)       */
#define GTA2_PAUSE_EXPLICIT_USER         6u /* player-requested pause             */
#define GTA2_PAUSE_MAX                   GTA2_PAUSE_EXPLICIT_USER

/* Overrun policy: when more than GTA2_SIM_MAX_CATCHUP_TICKS are overdue the
 * core enters a visible performance pause, discards the unrun wall-clock
 * debt (accumulator reset, never authoritative ticks) and requires explicit
 * resume. gta2_tick_advance and gta2_frame_pump return GTA2_STATUS_PAUSED
 * until gta2_sim_resume is called with the explicit UI token. */
#define GTA2_RESUME_TOKEN_EXPLICIT_UI 1u

/* gta2_frame_result: 8 uint32 = 32 bytes, written by the core. */
#define GTA2_FRAME_RESULT_BYTES 32u
#define GTA2_FRAME_RESULT_FIELD_STRUCT_BYTES   0u
#define GTA2_FRAME_RESULT_FIELD_TICKS_RUN      4u
#define GTA2_FRAME_RESULT_FIELD_TICK_INDEX     8u
#define GTA2_FRAME_RESULT_FIELD_ACCUMULATOR_MS 12u
#define GTA2_FRAME_RESULT_FIELD_ALPHA_Q16      16u /* accumulator_ms/33, always < 65536 */
#define GTA2_FRAME_RESULT_FIELD_PAUSE_REASON   20u
#define GTA2_FRAME_RESULT_FIELD_FLAGS          24u
#define GTA2_FRAME_RESULT_FIELD_RESERVED0      28u

#define GTA2_FRAME_FLAG_OVERRUN_OCCURRED 0x1u /* pause entered during this call */
#define GTA2_FRAME_FLAG_BASELINE_REQUIRED 0x2u /* caller must apply a full baseline */

/* gta2_sim_state: 8 uint32 = 32 bytes. */
#define GTA2_SIM_STATE_BYTES 32u
#define GTA2_SIM_STATE_FIELD_STRUCT_BYTES   0u
#define GTA2_SIM_STATE_FIELD_TICK_INDEX     4u
#define GTA2_SIM_STATE_FIELD_ACCUMULATOR_MS 8u
#define GTA2_SIM_STATE_FIELD_PAUSED         12u
#define GTA2_SIM_STATE_FIELD_PAUSE_REASON   16u
#define GTA2_SIM_STATE_FIELD_INPUT_ACCEPTED 20u
#define GTA2_SIM_STATE_FIELD_INPUT_REJECTED 24u
#define GTA2_SIM_STATE_FIELD_FLAGS          28u

#define GTA2_SIM_STATE_FLAG_BASELINE_REQUIRED 0x1u
#define GTA2_SIM_STATE_FLAG_INPUT_NEUTRALIZED 0x2u
#define GTA2_SIM_STATE_FLAG_TICK_IN_PROGRESS  0x4u

/* One frame callback from the platform clock. elapsed_ms is monotonic
 * wall-clock milliseconds since the previous call, clamped to
 * GTA2_SIM_MAX_FRAME_ELAPSED_MS. The core accumulates, runs at most four
 * ticks, and returns the interpolation alpha. */
gta2_status gta2_frame_pump(gta2_session session, uint32_t elapsed_ms,
                            void* out_result_bytes, uint32_t out_result_len);

/* Advance exactly one authoritative tick (deterministic tests, capture and
 * replay tooling). Refuses to run while paused, while a tick is in progress,
 * or in presentation mode; it never silently does nothing. */
gta2_status gta2_tick_advance(gta2_session session);

gta2_status gta2_sim_status(gta2_session session, void* out_state_bytes, uint32_t out_state_len);
gta2_status gta2_sim_pause(gta2_session session, uint32_t reason);
/* Explicit resume. Clears stale input edges and, for HOST/PRESENTATION,
 * requires a full baseline before ticks resume (D04). */
gta2_status gta2_sim_resume(gta2_session session, uint32_t resume_token);

/* ---------------- Input submission (D12) ---------------- */
/* frames_le is a batch of 1..3 GTA2_NET_INPUT_FRAME_BYTES frames, oldest
 * first (see gta2/net.h). Ownership is validated: local submission must match
 * the local slot, remote submission is host-only, values are bounded, the
 * epoch must match and sequences must be monotonically new per slot. */
gta2_status gta2_input_submit_local(gta2_session session, const uint8_t* frames_le, uint32_t length);
gta2_status gta2_input_submit_remote(gta2_session session, const uint8_t* frames_le, uint32_t length);

/* ---------------- Events (D03) ---------------- */
/* Drain buffer: a sequence of 16-byte headers plus typed payloads, all LE.
 * Unknown event types are forwarded, never discarded, so the shell can log a
 * visible defect (D12). */
#define GTA2_EVENT_HEADER_BYTES 16u
#define GTA2_EVENT_OFF_TYPE     0u  /* uint32 */
#define GTA2_EVENT_OFF_TICK     4u  /* uint32 */
#define GTA2_EVENT_OFF_SEQUENCE 8u  /* uint32 monotonic within the session */
#define GTA2_EVENT_OFF_PAYLOAD  12u /* uint32 typed payload bytes           */

#define GTA2_EVENT_SIM_PAUSED         1u
#define GTA2_EVENT_SIM_RESUMED        2u
#define GTA2_EVENT_PROFILE_REJECTED   3u
#define GTA2_EVENT_DISTRICT_BEGUN     4u
#define GTA2_EVENT_DISTRICT_ENDED     5u
#define GTA2_EVENT_SOUND_PLAY         6u
#define GTA2_EVENT_SOUND_STOP         7u
#define GTA2_EVENT_VIDEO_BEGIN        8u
#define GTA2_EVENT_VIDEO_END          9u
#define GTA2_EVENT_MISSION_STATE      10u
#define GTA2_EVENT_NET_BASELINE_REQUIRED 11u
#define GTA2_EVENT_NET_RESYNC_REQUIRED   12u
#define GTA2_EVENT_FATAL_NUMERIC      13u
#define GTA2_EVENT_UNSUPPORTED_DENIED 14u
#define GTA2_EVENT_GAME_ERROR         15u
#define GTA2_EVENT_MAX                GTA2_EVENT_GAME_ERROR

/* SOUND_PLAY payload: 32 bytes. gain_q8 0..256 maps to 0.0..1.0 linear;
 * pan_q8 -128..127; sample positions are 44.1 kHz frame indices as supplied
 * by the converted media; rate_q16 is 65536 = 1.0x; event_id deduplicates
 * repeated network events so a sound never plays twice (D08). */
#define GTA2_EVENT_SOUND_PLAY_BYTES 32u
#define GTA2_EV_SND_OFF_SOUND_ID        0u  /* uint32  */
#define GTA2_EV_SND_OFF_FLAGS           4u  /* uint32  */
#define GTA2_EV_SND_OFF_GAIN_Q8         8u  /* int16   */
#define GTA2_EV_SND_OFF_PAN_Q8          10u /* int16   */
#define GTA2_EV_SND_OFF_LOOP_START      12u /* uint32 sample frames */
#define GTA2_EV_SND_OFF_LOOP_END        16u /* uint32 sample frames, 0 = no loop */
#define GTA2_EV_SND_OFF_RATE_Q16        20u /* uint32  */
#define GTA2_EV_SND_OFF_VOICE_CLASS     24u /* uint32  */
#define GTA2_EV_SND_OFF_EVENT_ID        28u /* uint32  */

#define GTA2_EV_SOUND_FLAG_LOOPING 0x1u
#define GTA2_VOICE_CLASS_EFFECT      0u
#define GTA2_VOICE_CLASS_VOCAL       1u
#define GTA2_VOICE_CLASS_PLAYER_WEAPON 2u
#define GTA2_VOICE_CLASS_MISSION_SPEECH 3u
#define GTA2_VOICE_CLASS_UI          4u
#define GTA2_AUDIO_MAX_VOICES        64u
#define GTA2_AUDIO_SAMPLE_CACHE_BYTES 67108864u /* 64 MiB */

/* VIDEO_BEGIN payload: 16 bytes. A full-screen story video pauses world
 * simulation for every participant; the host's skip ends it for everyone. */
#define GTA2_EVENT_VIDEO_BEGIN_BYTES 16u
#define GTA2_EV_VID_OFF_ASSET_ID   0u /* uint32 */
#define GTA2_EV_VID_OFF_ASPECT_NUM 4u /* uint32 */
#define GTA2_EV_VID_OFF_ASPECT_DEN 8u /* uint32 */
#define GTA2_EV_VID_OFF_FLAGS      12u/* uint32 bit0 = pauses world simulation */
#define GTA2_EV_VIDEO_FLAG_PAUSES_WORLD 0x1u

/* FATAL_NUMERIC payload: 8 bytes. Divide by zero is a named fatal simulation
 * error with a saved reproduction; returning zero is forbidden (D05). */
#define GTA2_EVENT_FATAL_NUMERIC_BYTES 8u
#define GTA2_EV_FATAL_CODE_FIX16_DIVIDE_BY_ZERO 1u
#define GTA2_EV_FATAL_CODE_INT_MIN_DIV_MINUS_ONE 2u

gta2_status gta2_events_pending(gta2_session session, uint32_t* out_bytes);
gta2_status gta2_events_drain(gta2_session session, uint8_t* out, uint32_t capacity,
                              uint32_t* out_written);

#if defined(__cplusplus)
#define GTA2_PLATFORM_ASSERT(cond, msg) static_assert(cond, msg)
#else
#define GTA2_PLATFORM_ASSERT(cond, msg) _Static_assert(cond, msg)
#endif

GTA2_PLATFORM_ASSERT(GTA2_SESSION_DESC_FIELD_RESERVED2 + 4u == GTA2_SESSION_DESC_BYTES,
                     "session descriptor must be 32 bytes");
GTA2_PLATFORM_ASSERT(GTA2_FRAME_RESULT_FIELD_RESERVED0 + 4u == GTA2_FRAME_RESULT_BYTES,
                     "frame result must be 32 bytes");
GTA2_PLATFORM_ASSERT(GTA2_SIM_STATE_FIELD_FLAGS + 4u == GTA2_SIM_STATE_BYTES,
                     "sim state must be 32 bytes");
GTA2_PLATFORM_ASSERT(GTA2_EV_SND_OFF_EVENT_ID + 4u == GTA2_EVENT_SOUND_PLAY_BYTES,
                     "sound event must be 32 bytes");
GTA2_PLATFORM_ASSERT(GTA2_EVENT_OFF_PAYLOAD + 4u == GTA2_EVENT_HEADER_BYTES,
                     "event header must be 16 bytes");
GTA2_PLATFORM_ASSERT(GTA2_SIM_MAX_CATCHUP_TICKS <= 4u,
                     "catch-up cap is four ticks per callback (D04)");

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* GTA2_PLATFORM_H */
