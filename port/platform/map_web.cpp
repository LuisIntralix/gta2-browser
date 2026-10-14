// Map screen support (T33, a non-original feature): the district map image,
// built from the player's own map (GMP) and style (STY) files as loaded by
// the game, and the live state the shell's map and minimap need. Read-only:
// nothing here writes game state.
#include <emscripten.h>
#include <string.h>

#include "Camera.hpp"
#include "Car_BC.hpp"
#include "Game_0x40.hpp"
#include "Ped.hpp"
#include "Player.hpp"
#include "char.hpp"
#include "gtx_0x106C.hpp"
#include "map_0x370.hpp"
#include "Object_2C_Pool.hpp"
#include "Object_5C.hpp"
#include "Gang.hpp"
#include "Hud.hpp"
#include "ScriptManager_C1EA8.hpp"
#include "miss2_0x11C.hpp"
#include "sprite.hpp"

namespace
{
const Map_0x370* gLastMap = nullptr;
const Game_0x40* gLastGame = nullptr;
s32 gEpoch = 0;
u32 gMapId = 0;

Ped* PlayerPed()
{
    if (!gGame_0x40_67E008 || !gGame_0x40_67E008->field_38_orf1)
    {
        return nullptr;
    }
    return gGame_0x40_67E008->field_38_orf1->field_2C4_player_ped;
}

Sprite* PlayerSprite(Ped* ped)
{
    if (Car_BC* car = ped->get_car_416B60())
    {
        return car->field_50_car_sprite;
    }
    return ped->field_168_game_object ? ped->field_168_game_object->field_80_sprite_ptr : nullptr;
}

// The topmost block with a lid in a column, or null.
gmp_block_info* TopLid(s32 x, s32 y, s32* pZ)
{
    for (s32 z = 7; z >= 0; --z)
    {
        gmp_block_info* b = gMap_0x370_6F6268->get_block_4DFE10(x, y, z);
        if (b && (b->field_8_lid & 0x3FF))
        {
            *pZ = z;
            return b;
        }
    }
    return nullptr;
}

// Identifies the loaded district (for per-district discovery and waypoints).
u32 ComputeMapId()
{
    u32 h = 2166136261u;
    for (s32 y = 0; y < 256; y += 3)
    {
        for (s32 x = 0; x < 256; x += 3)
        {
            s32 z = 0;
            gmp_block_info* b = TopLid(x, y, &z);
            const u32 v = b ? (b->field_8_lid & 0x3FF) | (z << 10) : 0xFFFF;
            h = (h ^ v) * 16777619u;
        }
    }
    return h;
}
// Live objects in the whole pool. Objects that need no per-frame update
// (shops, triggers, many pickups) are allocated untracked and are not on the
// pool's used list, so this scans the pool array: a freed object has model 0
// and no sprite (Object_2C::PoolDeallocate).
template <typename F> void ForEachObject(F f)
{
    if (!gObject_2C_Pool_6F8F80)
    {
        return;
    }
    for (Object_2C& o : gObject_2C_Pool_6F8F80->field_8_pool)
    {
        if (o.field_18_model != 0 && o.field_4)
        {
            f(o);
        }
    }
}

} // namespace

extern "C" {

// Read by the game's HUD (Hud.cpp, port patch): skip drawing the gang/mission arrows.
s32 gGta2PortHideHudArrows = 0;

EMSCRIPTEN_KEEPALIVE void gta2_port_set_hud_arrows(int show)
{
    gGta2PortHideHudArrows = show ? 0 : 1;
}

// Where the last draw put the camera and the player (Source/port_interpolation.cpp).
extern "C" s32 gGta2PortDrawnCam[2];
extern "C" s32 gGta2PortDrawnPlayer[3];
extern "C" s32 gGta2PortInterpolate;

// [has map, map epoch (changes when a district loads), map id, has player,
//  player x, y, z (Fix16 raw), heading (Ang16, 0..1439; direction = (sin, cos)),
//  in car, camera x, y, drawn z, zoom (Fix16 raw), view width (reference px),
//  drawn player x, y, heading, drawn camera x, y (interpolated; 0 when off)]
EMSCRIPTEN_KEEPALIVE const s32* gta2_port_map_state()
{
    static s32 out[19];
    memset(out, 0, sizeof(out));
    const bool hasMap = gGame_0x40_67E008 && gMap_0x370_6F6268 && gGtx_0x106C_703DD4 &&
        gGtx_0x106C_703DD4->has_tiles_4C2EE0();
    if (!hasMap)
    {
        gLastMap = nullptr;
        gLastGame = nullptr;
        return out;
    }
    if (gMap_0x370_6F6268 != gLastMap || gGame_0x40_67E008 != gLastGame)
    {
        gLastMap = gMap_0x370_6F6268;
        gLastGame = gGame_0x40_67E008;
        gMapId = ComputeMapId();
        ++gEpoch;
    }
    out[0] = 1;
    out[1] = gEpoch;
    out[2] = static_cast<s32>(gMapId);
    if (Ped* ped = PlayerPed())
    {
        if (Sprite* s = PlayerSprite(ped))
        {
            out[3] = 1;
            out[4] = s->field_14_xy.x.mValue;
            out[5] = s->field_14_xy.y.mValue;
            out[6] = s->field_1C_zpos.mValue;
            out[7] = s->field_0.rValue;
            out[8] = ped->get_car_416B60() ? 1 : 0;
        }
    }
    if (const Camera_0xBC* c = gViewCamera_676978)
    {
        out[9] = c->field_98_cam_pos2.field_0_x.mValue;
        out[10] = c->field_98_cam_pos2.field_4_y.mValue;
        out[11] = Gta2Port_PresentedCameraZ(c).mValue;
        out[12] = c->field_98_cam_pos2.field_C_zoom.mValue;
        out[13] = c->field_68_screen_px_width;
    }
    if (gGta2PortInterpolate && out[3])
    {
        out[14] = gGta2PortDrawnPlayer[0];
        out[15] = gGta2PortDrawnPlayer[1];
        out[16] = gGta2PortDrawnPlayer[2];
        out[17] = gGta2PortDrawnCam[0];
        out[18] = gGta2PortDrawnCam[1];
    }
    return out;
}

// Renders `rows` block rows of the map, starting at block row y0, at px pixels
// per block (1..64), into out (RGBA, 256 * px wide). Each block shows the
// topmost lid, rotated and flipped as the map says, box-filtered down from
// 64x64. Transparent texels (colour 0) of "flat" lids - railings, rail tracks,
// fences - show the next lid below. Empty columns are transparent.
EMSCRIPTEN_KEEPALIVE int gta2_port_map_render_rows(int y0, int rows, int px, u8* out)
{
    if (!gMap_0x370_6F6268 || !gGtx_0x106C_703DD4 || !gGtx_0x106C_703DD4->has_tiles_4C2EE0() || px < 1 || px > 64 ||
        64 % px)
    {
        return 0;
    }
    gtx_0x106C* gtx = gGtx_0x106C_703DD4;
    const s32 step = 64 / px;
    const s32 pitch = 256 * px * 4;
    struct Layer
    {
        const u8* pixels;
        const u8* pal;
        u16 lid;
    };
    for (s32 by = y0; by < y0 + rows && by < 256; ++by)
    {
        for (s32 bx = 0; bx < 256; ++bx)
        {
            // lids in this column, top down, until the first one that is not see-through
            Layer layers[8];
            s32 count = 0;
            for (s32 z = 7; z >= 0 && count < 8; --z)
            {
                gmp_block_info* b = gMap_0x370_6F6268->get_block_4DFE10(bx, by, z);
                const u16 tile = b ? b->field_8_lid & 0x3FF : 0;
                if (!tile || tile > 992) // 992: the original's out-of-range tile read (see TextureCache_15D8)
                {
                    continue;
                }
                layers[count++] = {gtx->get_tile_4C2EB0(tile), gtx->GetPalData_5AA6A0(gtx->get_phys_pal_5AA6F0(tile)), b->field_8_lid};
                if (!((b->field_8_lid >> 12) & 1))
                {
                    break; // solid lid: nothing below shows
                }
            }
            u8* dst0 = out + (by - y0) * px * pitch + bx * px * 4;
            for (s32 y = 0; y < px; ++y)
            {
                u8* dst = dst0 + y * pitch;
                for (s32 x = 0; x < px; ++x)
                {
                    u32 r = 0, g = 0, bl = 0, n = 0, total = 0;
                    for (s32 sy = 0; sy < step; ++sy)
                    {
                        for (s32 sx = 0; sx < step; ++sx)
                        {
                            ++total;
                            const s32 u = x * step + sx, v = y * step + sy;
                            for (s32 l = 0; l < count; ++l)
                            {
                                const u16 lid = layers[l].lid;
                                // destination texel -> tile texel: undo the rotation, then the flip
                                s32 tu, tv;
                                switch ((lid >> 14) & 3)
                                {
                                    case 1: tu = v; tv = 63 - u; break;
                                    case 2: tu = 63 - u; tv = 63 - v; break;
                                    case 3: tu = 63 - v; tv = u; break;
                                    default: tu = u; tv = v; break;
                                }
                                if ((lid >> 13) & 1)
                                {
                                    tu = 63 - tu;
                                }
                                const u8 index = layers[l].pixels[tv * 256 + tu];
                                const bool flat = (lid >> 12) & 1;
                                if (index == 0 && flat && l + 1 < count)
                                {
                                    continue; // see-through: use the lid below
                                }
                                if (index == 0 && flat)
                                {
                                    break; // see-through with nothing below
                                }
                                // 32-bit palette entries, stored as columns 256 bytes apart (B, G, R, x)
                                const u8* c = layers[l].pal + index * 256;
                                r += c[2];
                                g += c[1];
                                bl += c[0];
                                ++n;
                                break;
                            }
                        }
                    }
                    dst[x * 4 + 0] = static_cast<u8>(n ? r / n : 0);
                    dst[x * 4 + 1] = static_cast<u8>(n ? g / n : 0);
                    dst[x * 4 + 2] = static_cast<u8>(n ? bl / n : 0);
                    dst[x * 4 + 3] = static_cast<u8>(n * 255 / total);
                }
            }
        }
    }
    return 1;
}

// Points of interest: the game's own objects of the kinds a map shows, as
// [model, x, y, z (Fix16 raw), extra] per object, where extra is the object's
// sub-type byte (field_26: for car shops, what the shop does). Returns the
// count (at most max). Models (enums.hpp, namespace objects): 161 trigger
// area, 164/177/179/181 phones, 130/131/162/163 car shops, 142/143 crusher,
// 200-229 weapon pickups, 230-249 power-ups, 250-252 car-weapon shops,
// 266 secret token, 286 kill frenzy.
EMSCRIPTEN_KEEPALIVE int gta2_port_map_objects(s32* out, int max)
{
    int n = 0;
    ForEachObject([&](Object_2C& o) {
        const s32 m = o.field_18_model;
        const bool wanted = m == 161 || m == 164 || m == 177 || m == 179 || m == 181 || m == 130 || m == 131 ||
            m == 162 || m == 163 || m == 142 || m == 143 || (m >= 200 && m <= 252) || m == 266 || m == 286;
        if (!wanted || n >= max)
        {
            return;
        }
        out[n * 5 + 0] = m;
        out[n * 5 + 1] = o.field_4->field_14_xy.x.mValue;
        out[n * 5 + 2] = o.field_4->field_14_xy.y.mValue;
        out[n * 5 + 3] = o.field_4->field_1C_zpos.mValue;
        out[n * 5 + 4] = o.field_26_ped_ref_idx;
        ++n;
    });
    return n;
}

// Developer probe: number of live objects per model (0..299) into out[300].
EMSCRIPTEN_KEEPALIVE int gta2_dev_object_models(s32* out)
{
    memset(out, 0, 300 * sizeof(s32));
    int n = 0;
    ForEachObject([&](Object_2C& o) {
        if (o.field_18_model >= 0 && o.field_18_model < 300)
        {
            ++out[o.field_18_model];
        }
        ++n;
    });
    return n;
}

// Script trigger areas (the mission script's "wait for the player in this
// area" threads), as [x, y, z (Fix16 raw), is save point] each. A trigger is
// a save point when its thread reaches SAVE_GAME / DO_SAVE_GAME within its
// first commands (church saves). Returns the count.
EMSCRIPTEN_KEEPALIVE int gta2_port_map_triggers(s32* out, int max)
{
    if (!gCar_214_705F20 || !gScriptManager_6F8060)
    {
        return 0;
    }
    int n = 0;
    for (Car_18& t : gCar_214_705F20->field_0_triggers)
    {
        if (n >= max || !t.field_8_type || !t.field_4_O2C || !t.field_4_O2C->field_4 || !t.field_0_pScriptCmd)
        {
            continue;
        }
        const SCR_THREAD* thread = reinterpret_cast<const SCR_THREAD*>(t.field_0_pScriptCmd);
        bool save = false;
        u16 idx = thread->field_E_thread_start_cmd;
        for (s32 step = 0; step < 48 && idx; ++step)
        {
            const SCR_CMD_HEADER* cmd = gScriptManager_6F8060->GetBasePointer_512770(idx);
            if (!cmd)
            {
                break;
            }
            if (cmd->field_2_type == SCRCMD_SAVE_GAME || cmd->field_2_type == SCRCMD_DO_SAVE_GAME)
            {
                save = true;
                break;
            }
            if (cmd->field_4_cmd_next <= 0 || static_cast<u16>(cmd->field_4_cmd_next) == idx)
            {
                break;
            }
            idx = static_cast<u16>(cmd->field_4_cmd_next);
        }
        const Sprite* sp = t.field_4_O2C->field_4;
        out[n * 4 + 0] = sp->field_14_xy.x.mValue;
        out[n * 4 + 1] = sp->field_14_xy.y.mValue;
        out[n * 4 + 2] = sp->field_1C_zpos.mValue;
        out[n * 4 + 3] = save ? 1 : 0;
        ++n;
    }
    return n;
}

// Gangs in this district, as [index, arrow colour, info phone x, y, z
// (Fix16 raw), name code (3 ASCII letters packed little-endian)] each.
EMSCRIPTEN_KEEPALIVE int gta2_port_map_gangs(s32* out, int max)
{
    if (!gGangPool_CA8_67E274)
    {
        return 0;
    }
    int n = 0;
    for (s32 i = 0; i < 10 && n < max; ++i)
    {
        Gang_144& g = gGangPool_CA8_67E274->field_0_gang_list[i];
        if (!g.field_0_used)
        {
            continue;
        }
        out[n * 6 + 0] = i;
        out[n * 6 + 1] = g.field_138_arrow_colour;
        out[n * 6 + 2] = g.field_12C_info_phone_x.mValue;
        out[n * 6 + 3] = g.field_130_info_phone_y.mValue;
        out[n * 6 + 4] = g.field_134_info_phone_z.mValue;
        out[n * 6 + 5] = static_cast<u8>(g.field_2_name[0]) | (static_cast<u8>(g.field_2_name[1]) << 8) |
            (static_cast<u8>(g.field_2_name[2]) << 16);
        ++n;
    }
    return n;
}

// The HUD's gang and mission arrows (what they point at), as [gang index or
// -1, arrow colour, target kind (ArrowTargetType: 1 position, 2 person, 3 car,
// 4 object, 5 info phone, 6 player), x, y, z (Fix16 raw), shown, required
// respect, the player's respect with that gang, on a mission, arrow sprite]
// per target.
// Arrow colours: 1 yellow (medium job), 2 red (hard), 3 green (easy), 4 info
// phone, 5 flashing (mission target). "shown" is the game's own visibility
// decision for the arrow (Hud_Arrow_7C::CheckVisibility_5D0530: no job arrows
// on a mission, respect thresholds, ...), for the arrow's current target.
// Both targets of an arrow are listed. Returns the count.
EMSCRIPTEN_KEEPALIVE int gta2_port_map_arrows(s32* out, int max)
{
    if (!gHud_2B00_706620)
    {
        return 0;
    }
    int n = 0;
    Player* player = gGame_0x40_67E008 ? gGame_0x40_67E008->field_38_orf1 : nullptr;
    const bool onMission = gScriptManager_6F8060 && gScriptManager_6F8060->IsOnMission_4C7350();
    for (Hud_Arrow_7C& a : gHud_2B00_706620->field_1F18_arrows.field_0_array)
    {
        Hud_ArrowTargets_64& t = a.field_18;
        s32 gang = -1;
        if (t.field_10.field_30_gang && gGangPool_CA8_67E274)
        {
            gang = static_cast<s32>(t.field_10.field_30_gang - gGangPool_CA8_67E274->field_0_gang_list);
            if (gang < 0 || gang >= 10)
            {
                gang = -1;
            }
        }
        ArrowTrace_24* const traces[2] = {&t.field_18_primary_target, &t.field_3C_secondary_target};
        for (ArrowTrace_24* tr : traces)
        {
            if (n >= max || tr->HasNoTarget_4C6F20() || tr->field_10_target_type == ArrowTargetType::Player_6)
            {
                continue;
            }
            s32* row = out + n * 11;
            row[0] = gang;
            row[1] = t.field_28_arrow_colour;
            row[2] = tr->field_10_target_type;
            row[3] = tr->field_14_aim_x.mValue;
            row[4] = tr->field_18_aim_y.mValue;
            row[5] = tr->field_1C_aim_z.mValue;
            row[6] = (a.IsVisible_4C7050() && t.field_60_curr_target == tr) ? 1 : 0;
            row[7] = t.field_10.field_34_min_respect;
            row[8] = (t.field_10.field_30_gang && player) ? t.field_10.field_30_gang->GetRespectForPlayer_4BEEF0(player->get_idx_4219D0()) : 0;
            row[9] = onMission ? 1 : 0;
            row[10] = t.field_2C_arrow_sprt_idx;
            ++n;
        }
    }
    return n;
}

// A sprite as RGBA (colour 0 transparent), the way the HUD draws it unremapped
// (TextureCache_15D8: sprite palettes), for the map's arrows and gang logos.
// sprite_type as in DrawFigure (6: HUD sprites). Returns (width << 16) |
// height, or 0 if it does not fit in max bytes.
EMSCRIPTEN_KEEPALIVE int gta2_port_sprite_rgba(int sprite_type, int sprite_idx, u8* out, int max)
{
    gtx_0x106C* gtx = gGtx_0x106C_703DD4;
    if (!gtx || !gtx->has_tiles_4C2EE0())
    {
        return 0;
    }
    const u16 index = gtx->GetSpriteTrueIndex_5AA460(sprite_type, static_cast<s16>(sprite_idx));
    const sprite_index* si = gtx->get_sprite_index_5AA440(index);
    const s32 w = si->field_4_width, h = si->field_5_height;
    if (!si->field_0_pData || w * h * 4 > max)
    {
        return 0;
    }
    const u8* pal = gtx->GetPalData_5AA6A0(gtx->get_phys_pal_5AA6F0(gtx->GetTruePalette_5AA5F0(palette_types_enum::sprites_2, index)));
    for (s32 y = 0; y < h; ++y)
    {
        for (s32 x = 0; x < w; ++x)
        {
            const u8 c = si->field_0_pData[y * 256 + x];
            const u8* e = pal + c * 256;
            u8* o = out + (y * w + x) * 4;
            o[0] = e[2];
            o[1] = e[1];
            o[2] = e[0];
            o[3] = c ? 255 : 0;
        }
    }
    return (w << 16) | h;
}

} // extern "C"
