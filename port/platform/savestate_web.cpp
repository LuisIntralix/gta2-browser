// Quick save and quick load (F5 / F9, single player): a snapshot of the game's whole memory.
//
// Everything the game knows lives in the WebAssembly memory, so a save is a copy of it up to
// the heap's end, taken between frames by web/savestate.js. What lives outside it in the
// browser is rebuilt after a load: the GPU textures (gbh_web.cpp), the sounds (miles_web.cpp),
// the clock (continues from the saved time), held keys and the frame timer.
#include <emscripten.h>
#include <emscripten/heap.h>
#include <string.h>
#include <unistd.h>

#include "Game_0x40.hpp"
#include "GameSession_578.hpp"
#include "Ped.hpp"
#include "Player.hpp"
#include "ScriptManager_C1EA8.hpp"
#include "map_0x370.hpp"
#include "winmain.hpp"

extern "C" void gta2_port_set_clock(double ms);
extern "C" double gta2_port_now_ms();
extern "C" void gta2_port_release_all_keys();
void Gta2Port_ReuploadTextures();
void Gta2Port_AudioBeforeSave();
void Gta2Port_AudioAfterRestore();

extern "C" {

// 1 when there is something to save: a single-player game in progress (not the menus, not co-op).
EMSCRIPTEN_KEEPALIVE int gta2_port_savestate_can_save()
{
    return gGame_0x40_67E008 && !bStartNetworkGame_7081F0 ? 1 : 0;
}

// The Fair saving rules (web/savestate.js): [player on foot or in a car, cash, wanted stars,
// on a job, jobs passed, kill frenzies passed]. "On a job" is the script's mission flag, which
// also hides the phone arrows; the two counts are the ones the status screen shows (frenzies are
// the script's "secrets"). Either rising triggers the autosave.
EMSCRIPTEN_KEEPALIVE const s32* gta2_port_savestate_rules()
{
    static s32 rules[6];
    memset(rules, 0, sizeof(rules));
    Player* p = gGame_0x40_67E008 ? gGame_0x40_67E008->field_38_orf1 : nullptr;
    if (p && p->field_2C4_player_ped)
    {
        rules[0] = 1;
        rules[1] = p->GetScore_421980();
        rules[2] = p->field_2C4_player_ped->get_wanted_star_count_46EF00();
        rules[3] = gScriptManager_6F8060 && gScriptManager_6F8060->IsOnMission_4C7350() ? 1 : 0;
        rules[4] = gScriptManager_6F8060 && gScriptManager_6F8060->field_328_passed_flag ? *gScriptManager_6F8060->field_328_passed_flag : 0;
        rules[5] = gScriptManager_6F8060 && gScriptManager_6F8060->field_338_secrets_passed ? *gScriptManager_6F8060->field_338_secrets_passed : 0;
    }
    return rules;
}

// A save's price, taken before the copy (so the save holds the cash after paying); a negative
// amount refunds a save that could not be stored.
EMSCRIPTEN_KEEPALIVE void gta2_port_savestate_pay(int cash)
{
    Player* p = gGame_0x40_67E008 ? gGame_0x40_67E008->field_38_orf1 : nullptr;
    if (p)
    {
        p->AddCash_421990(-cash);
    }
}

// Before the copy: stream positions go into memory. Returns how many bytes to save (the heap's
// end) and the game clock to continue from.
EMSCRIPTEN_KEEPALIVE unsigned gta2_port_savestate_prepare(double* clock)
{
    Gta2Port_AudioBeforeSave();
    *clock = gta2_port_now_ms();
    return static_cast<unsigned>(reinterpret_cast<uintptr_t>(sbrk(0)));
}

// Before a load: grows the memory to hold the snapshot. 0 if the browser refuses.
EMSCRIPTEN_KEEPALIVE int gta2_port_savestate_reserve(unsigned bytes)
{
    return emscripten_get_heap_size() >= bytes || emscripten_resize_heap(bytes) ? 1 : 0;
}

// After the memory was replaced: rebuild what lives in the browser.
EMSCRIPTEN_KEEPALIVE void gta2_port_savestate_restored(double clock)
{
    gta2_port_set_clock(clock);
    Gta2Port_ReuploadTextures();
    Gta2Port_AudioAfterRestore();
    gta2_port_release_all_keys(); // keys held when saving are not held now
    ResetFrameTimer_4DA830();     // the time spent loading is not simulated
}

// What a save's card shows, read after the copy (finding the area moves the map's zone cursor):
// [has player, cash, lives, multiplier, wanted stars, health, in a car, 0], then the area's name
// (UTF-16, as the game shows it on screen) and the map file, each 0-terminated.
struct SaveLabel
{
    s32 stats[8];
    u16 area[48];
    char map[64];
};
EMSCRIPTEN_KEEPALIVE const SaveLabel* gta2_port_savestate_label()
{
    static SaveLabel label;
    memset(&label, 0, sizeof(label));
    Player* p = gGame_0x40_67E008 ? gGame_0x40_67E008->field_38_orf1 : nullptr;
    if (!p)
    {
        return &label;
    }
    strncpy(label.map, gGameSession_67E8E0.GetMapName_4C5940(), sizeof(label.map) - 1);
    label.stats[0] = 1;
    label.stats[1] = p->GetScore_421980();
    label.stats[2] = p->get_lives_4766C0();
    label.stats[3] = p->get_multiplier_4766A0();
    if (Ped* ped = p->field_2C4_player_ped)
    {
        label.stats[4] = ped->get_wanted_star_count_46EF00();
        label.stats[5] = ped->field_216_health;
        label.stats[6] = ped->get_car_416B60() != nullptr;
    }
    if (gMap_0x370_6F6268)
    {
        u8 x, y, z;
        p->GetPosU8_569840(x, y, z);
        gmp_map_zone* zone = gMap_0x370_6F6268->zone_by_pos_and_type_4DF4D0(x, y, Local_Navigation_15);
        if (!zone)
        {
            zone = gMap_0x370_6F6268->zone_by_pos_and_type_4DF4D0(x, y, Navigation_1);
        }
        if (const wchar_t* name = zone ? zone->get_zone_str_4DEF00() : nullptr)
        {
            for (size_t i = 0; i < 47 && name[i]; ++i)
            {
                label.area[i] = static_cast<u16>(name[i]);
            }
        }
    }
    return &label;
}

} // extern "C"
