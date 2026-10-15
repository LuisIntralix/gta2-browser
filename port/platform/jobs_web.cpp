// The current job's instructions for the shell's map screen (M). GTA2 shows a job's
// instructions only as briefs (the text with a gang face at the bottom of the screen) that
// scroll away; the port keeps the latest ones here so the map can show them again.
//
// Called once per frame. The log lives in the game's memory, so a save keeps it.
// - Every brief is logged (a repeated one moves to the end), newest last, at most kMaxBriefs.
// - When a job starts (the script's mission flag, which also hides the phone arrows), briefs
//   older than kJobLeadMs go: the job's first instructions come with the phone call, just
//   before the flag is set.
#include <emscripten.h>
#include <string.h>
#include <wchar.h>

#include "Game_0x40.hpp"
#include "Hud.hpp"
#include "ScriptManager_C1EA8.hpp"
#include "text_0x14.hpp"

extern "C" double gta2_port_now_ms();

namespace
{
constexpr int kMaxBriefs = 8;
constexpr int kBriefLen = 320;
constexpr double kJobLeadMs = 8000;

// Read by web/map.js (gta2_port_job_briefs): [on a job, count, face per brief...], then the
// texts (UTF-16, 0-terminated), oldest first.
struct JobBriefs
{
    s32 onJob;
    s32 count;
    s32 face[kMaxBriefs];
    double shownAt[kMaxBriefs];
    wchar_t text[kMaxBriefs][kBriefLen];
};
JobBriefs gJob;

// The brief last seen on screen, so it is formatted and logged once.
char gSeenKey[8];
s32 gSeenParam = -2;

void Drop(int i)
{
    for (int j = i; j + 1 < gJob.count; ++j)
    {
        gJob.face[j] = gJob.face[j + 1];
        gJob.shownAt[j] = gJob.shownAt[j + 1];
        memcpy(gJob.text[j], gJob.text[j + 1], sizeof(gJob.text[j]));
    }
    --gJob.count;
}

void Log(const wchar_t* text, s32 face, double now)
{
    for (int i = 0; i < gJob.count; ++i)
    {
        if (!wcscmp(gJob.text[i], text))
        {
            Drop(i); // shown again: it moves to the end
            break;
        }
    }
    if (gJob.count == kMaxBriefs)
    {
        Drop(0);
    }
    wcsncpy(gJob.text[gJob.count], text, kBriefLen - 1);
    gJob.text[gJob.count][kBriefLen - 1] = 0;
    gJob.face[gJob.count] = face;
    gJob.shownAt[gJob.count] = now;
    ++gJob.count;
}
} // namespace

void Gta2Port_TrackJobBriefs()
{
    if (!gGame_0x40_67E008 || !gHud_2B00_706620 || !gText_0x14_704DFC)
    {
        return;
    }
    const double now = gta2_port_now_ms();
    const s32 onJob = gScriptManager_6F8060 && gScriptManager_6F8060->IsOnMission_4C7350() ? 1 : 0;
    if (onJob && !gJob.onJob)
    {
        for (int i = gJob.count - 1; i >= 0; --i)
        {
            if (now - gJob.shownAt[i] > kJobLeadMs)
            {
                Drop(i);
            }
        }
    }
    gJob.onJob = onJob;

    const Hud_BriefEntry_18* brief = gHud_2B00_706620->field_DC_brief.field_6F8_curr_briefs;
    if (!brief)
    {
        gSeenKey[0] = 0;
        return;
    }
    if (!strncmp(gSeenKey, brief->field_0_brief_id_str, sizeof(gSeenKey)) && gSeenParam == brief->field_14_cost_param)
    {
        return;
    }
    strncpy(gSeenKey, brief->field_0_brief_id_str, sizeof(gSeenKey));
    gSeenParam = brief->field_14_cost_param;

    // as Hud_Brief_704::FormatCurrentBrief_5D3470, without the line breaks
    static wchar_t text[1024]; // briefs are a few lines; the game formats them into tmpBuff_67BD9C
    const wchar_t* found = gText_0x14_704DFC->Find_5B5F90(brief->field_0_brief_id_str);
    if (!found)
    {
        return;
    }
    if (brief->field_14_cost_param != -1)
    {
        swprintf(text, found, brief->field_14_cost_param); // the port's swprintf is the MSVC form, as the game's
    }
    else
    {
        wcsncpy(text, found, 1023);
        text[1023] = 0;
    }
    const s32 face = GetBriefFaceIdx_5D3680(text[0]);
    Log(face ? text + 1 : text, face ? face : 8, now); // a face is "!z" etc. at the start
}

extern "C" EMSCRIPTEN_KEEPALIVE const JobBriefs* gta2_port_job_briefs()
{
    return &gJob;
}
