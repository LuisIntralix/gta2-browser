// The front end's Options item (issue #2). The original launched the separate GTA2 Manager
// program to change the controls; in the browser the shell shows its own Controls screen
// (web/controls.js), which writes the same registry values and has the game read them again.
#include <emscripten.h>

#include "InputRecorder_67F8B0.hpp"

EM_JS(void, gta2_js_open_options, (), { if (Module.onOpenOptions) Module.onOpenOptions(); });

// winmain.cpp: Start_GTA2Manager_5E4DE0
void Gta2Port_OpenOptions()
{
    gta2_js_open_options();
}

extern "C" {

// After the Controls screen changed hklm\...\gta2\control: the game reads its key bindings again.
EMSCRIPTEN_KEEPALIVE void gta2_port_reload_controls()
{
    if (gInputDevices_67B990)
    {
        gInputDevices_67B990->get_registry_controls_498C00();
    }
}

} // extern "C"
