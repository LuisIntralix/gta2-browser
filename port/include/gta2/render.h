/* GTA2 browser port — versioned render view contract v1 (T04)
 *
 * Binding decisions: D03 (one boundary crossing per frame; JS performs WebGL
 * calls), D07 (graphics and modern presentation), D04 (interpolation between
 * previous and current state), D08 (audio events are separate, see platform.h).
 * Contract: docs/contracts/render-view-v1.md
 *
 * The core never issues a browser call. It publishes one versioned linear
 * buffer per frame; JavaScript reads it and performs the WebGL2 calls. All
 * offsets are byte offsets from the start of the view and every record is a
 * multiple of 4 bytes, so the layout is identical for every compiler. No
 * struct is copied into the buffer and no pointer is stored in it.
 */
#ifndef GTA2_RENDER_H
#define GTA2_RENDER_H

#include "gta2/abi.h"
#include "gta2/fixed.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GTA2_RENDER_VIEW_VERSION 1u

/* ---------------- View header: 16 uint32 = 64 bytes ---------------- */
#define GTA2_RENDER_HEADER_BYTES   64u
#define GTA2_R_OFF_VERSION         0u  /* uint32, GTA2_RENDER_VIEW_VERSION      */
#define GTA2_R_OFF_TICK            4u  /* uint32, authoritative tick of `current`*/
#define GTA2_R_OFF_FLAGS           8u  /* uint32, bit0 = has_previous_state      */
#define GTA2_R_OFF_BYTE_LENGTH     12u /* uint32, total view bytes incl. header  */
#define GTA2_R_OFF_CAMERA_OFFSET   16u /* uint32, byte offset of camera records  */
#define GTA2_R_OFF_CAMERA_COUNT    20u /* uint32                                 */
#define GTA2_R_OFF_COMMAND_OFFSET  24u /* uint32, byte offset of command records */
#define GTA2_R_OFF_COMMAND_COUNT   28u /* uint32                                 */
#define GTA2_R_OFF_VERTEX_OFFSET   32u /* uint32, byte offset of vertices        */
#define GTA2_R_OFF_VERTEX_COUNT    36u /* uint32                                 */
#define GTA2_R_OFF_INDEX_OFFSET    40u /* uint32, byte offset of uint32 indices  */
#define GTA2_R_OFF_INDEX_COUNT     44u /* uint32                                 */
#define GTA2_R_OFF_ENTITY_OFFSET   48u /* uint32, byte offset of entity records  */
#define GTA2_R_OFF_ENTITY_COUNT    52u /* uint32                                 */
#define GTA2_R_OFF_RESERVED0       56u /* uint32, must be 0                       */
#define GTA2_R_OFF_RESERVED1       60u /* uint32, must be 0                       */

#define GTA2_RENDER_FLAG_HAS_PREVIOUS_STATE 0x1u

/* ---------------- Camera record: 8 uint32 = 32 bytes ----------------
 * World coordinates and spans are Fix16 reference-space units (D05, D07).
 * The canonical gameplay/activation rectangle is separate from presentation
 * culling: widescreen must not activate missions, alter AI range or
 * accelerate despawns. */
#define GTA2_RENDER_CAMERA_BYTES      32u
#define GTA2_R_CAM_OFF_ID             0u  /* uint32 camera/player id              */
#define GTA2_R_CAM_OFF_FLAGS          4u  /* uint32 bit0 = canonical gameplay rect */
#define GTA2_R_CAM_OFF_CENTER_X       8u  /* int32 Fix16 reference units          */
#define GTA2_R_CAM_OFF_CENTER_Y       12u /* int32 Fix16 reference units          */
#define GTA2_R_CAM_OFF_SPAN_HEIGHT    16u /* int32 Fix16, 480 * ONE by default    */
#define GTA2_R_CAM_OFF_SPAN_WIDTH     20u /* int32 Fix16, span_height * aspect    */
#define GTA2_R_CAM_OFF_SCREEN_W       24u /* uint32 output pixels                 */
#define GTA2_R_CAM_OFF_SCREEN_H       28u /* uint32 output pixels                 */

#define GTA2_RENDER_CAMERA_FLAG_CANONICAL_GAMEPLAY 0x1u

/* ---------------- Command record: 8 uint32 = 32 bytes ----------------
 * Commands are emitted in reference draw order. Only consecutive commands
 * with identical state may be merged; transparent sprites are never globally
 * re-sorted (D07). */
#define GTA2_RENDER_COMMAND_BYTES     32u
#define GTA2_R_CMD_OFF_KIND           0u  /* uint32 GTA2_RENDER_CMD_*             */
#define GTA2_R_CMD_OFF_TEXTURE_ID     4u  /* uint32 atlas id, 0 = none            */
#define GTA2_R_CMD_OFF_BLEND          8u  /* uint32 GTA2_RENDER_BLEND_*           */
#define GTA2_R_CMD_OFF_DEPTH          12u /* uint32 GTA2_RENDER_DEPTH_*           */
#define GTA2_R_CMD_OFF_INDEX_OFFSET   16u /* uint32 byte offset into index section*/
#define GTA2_R_CMD_OFF_INDEX_COUNT    20u /* uint32 index count                   */
#define GTA2_R_CMD_OFF_LAYER          24u /* uint32 0=world 1=hud 2=letterbox     */
#define GTA2_R_CMD_OFF_FLAGS          28u /* uint32 bit0 = screen space           */

#define GTA2_RENDER_CMD_SPRITE        1u  /* indexed quads, textured              */
#define GTA2_RENDER_CMD_SOLID_QUAD    2u  /* indexed quads, untextured            */
#define GTA2_RENDER_CMD_HUD_QUAD      3u  /* screen-space quad, HUD ortho layer   */

#define GTA2_RENDER_BLEND_OPAQUE      0u
#define GTA2_RENDER_BLEND_ALPHA       1u
#define GTA2_RENDER_BLEND_ADDITIVE    2u
#define GTA2_RENDER_BLEND_MASKED      3u

#define GTA2_RENDER_DEPTH_NONE        0u
#define GTA2_RENDER_DEPTH_LESS        1u
#define GTA2_RENDER_DEPTH_LESS_EQUAL  2u

#define GTA2_RENDER_LAYER_WORLD       0u
#define GTA2_RENDER_LAYER_HUD         1u
#define GTA2_RENDER_LAYER_LETTERBOX   2u

/* ---------------- Vertex: 16 bytes ----------------
 * x/y are Fix16 reference-space coordinates; u/v are int16 atlas texel
 * coordinates (nearest sampling, 1-pixel duplicated border); rgba is 8-bit
 * per channel in memory order R,G,B,A. */
#define GTA2_RENDER_VERTEX_BYTES      16u
#define GTA2_R_VERT_OFF_X             0u  /* int32 Fix16 */
#define GTA2_R_VERT_OFF_Y             4u  /* int32 Fix16 */
#define GTA2_R_VERT_OFF_U             8u  /* int16       */
#define GTA2_R_VERT_OFF_V             10u /* int16       */
#define GTA2_R_VERT_OFF_RGBA8         12u /* uint32 R|G<<8|B<<16|A<<24 */

#define GTA2_RENDER_INDEX_BYTES       4u  /* uint32 indices; no 16-bit truncation */

/* ---------------- Entity interpolation record: 32 bytes ----------------
 * JavaScript renders between previous and current state using
 * alpha = accumulator_ms / 33, supplied by gta2_frame_pump (D04). The core
 * publishes both states; it does not interpolate for the renderer. */
#define GTA2_RENDER_ENTITY_BYTES      32u
#define GTA2_R_ENT_OFF_ID             0u  /* uint32 entity id                     */
#define GTA2_R_ENT_OFF_GENERATION     4u  /* uint32 generation                    */
#define GTA2_R_ENT_OFF_PREV_X         8u  /* int32 Fix16                          */
#define GTA2_R_ENT_OFF_PREV_Y         12u /* int32 Fix16                          */
#define GTA2_R_ENT_OFF_CUR_X          16u /* int32 Fix16                          */
#define GTA2_R_ENT_OFF_CUR_Y          20u /* int32 Fix16                          */
#define GTA2_R_ENT_OFF_PREV_ANGLE     24u /* uint16 gta2_ang16                    */
#define GTA2_R_ENT_OFF_CUR_ANGLE      26u /* uint16 gta2_ang16                    */
#define GTA2_R_ENT_OFF_FLAGS          28u /* uint32 bit0 = freeze (no extrapolation) */
#define GTA2_R_ENT_OFF_RESERVED0      30u /* uint16 must be 0                     */

#define GTA2_RENDER_ENTITY_FLAG_FREEZE 0x1u

/* ---------------- Presentation constants (D07) ---------------- */
#define GTA2_RENDER_REFERENCE_WIDTH      640u
#define GTA2_RENDER_REFERENCE_HEIGHT     480u
#define GTA2_RENDER_VERTICAL_SPAN        480        /* Fix16 whole units preserved */
#define GTA2_RENDER_ATLAS_WIDTH          2048u
#define GTA2_RENDER_ATLAS_HEIGHT         2048u
#define GTA2_RENDER_ATLAS_BORDER_PIXELS  1u
#define GTA2_RENDER_SAMPLING_NEAREST     1u
#define GTA2_RENDER_TEXTURE_CACHE_BYTES  134217728u /* 128 MiB */
#define GTA2_RENDER_MAX_VIEW_PIXELS      (2560u * 1440u)
#define GTA2_RENDER_HUD_SCALE_MIN        0.75
#define GTA2_RENDER_HUD_SCALE_DEFAULT    1.0
#define GTA2_RENDER_HUD_SCALE_MAX        1.5
#define GTA2_RENDER_NO_AI_UPSCALING      1u /* game art is never replaced or upscaled */

/* Output resolution policy: render at the selected 1080p/1440p pixel
 * dimensions; dynamic resolution is not an available optimisation (D15). */
#define GTA2_RENDER_RES_1080P_W 1920u
#define GTA2_RENDER_RES_1080P_H 1080u
#define GTA2_RENDER_RES_1440P_W 2560u
#define GTA2_RENDER_RES_1440P_H 1440u
#define GTA2_RENDER_DYNAMIC_RESOLUTION 0u

/* Core-owned render view descriptor returned by gta2_render_view. `data` and
 * `length` cover exactly GTA2_R_OFF_BYTE_LENGTH bytes. Valid until the next
 * gta2_* call; JavaScript must copy or reacquire, never retain (D03). */
typedef struct gta2_render_view {
    const uint8_t* data;
    uint32_t       length;
    uint32_t       version;
    uint32_t       reserved0;
} gta2_render_view;

gta2_status gta2_render_view_get(gta2_session session, gta2_render_view* out_view);

#if defined(__cplusplus)
#define GTA2_RENDER_ASSERT(cond, msg) static_assert(cond, msg)
#else
#define GTA2_RENDER_ASSERT(cond, msg) _Static_assert(cond, msg)
#endif

GTA2_RENDER_ASSERT(GTA2_R_OFF_RESERVED1 + 4u == GTA2_RENDER_HEADER_BYTES,
                   "render header must be 64 bytes");
GTA2_RENDER_ASSERT(GTA2_R_CMD_OFF_FLAGS + 4u == GTA2_RENDER_COMMAND_BYTES,
                   "command record must be 32 bytes");
GTA2_RENDER_ASSERT(GTA2_R_CAM_OFF_SCREEN_H + 4u == GTA2_RENDER_CAMERA_BYTES,
                   "camera record must be 32 bytes");
GTA2_RENDER_ASSERT(GTA2_R_VERT_OFF_RGBA8 + 4u == GTA2_RENDER_VERTEX_BYTES,
                   "vertex record must be 16 bytes");
GTA2_RENDER_ASSERT(GTA2_R_ENT_OFF_RESERVED0 + 2u == GTA2_RENDER_ENTITY_BYTES,
                   "entity record must be 32 bytes");

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* GTA2_RENDER_H */
