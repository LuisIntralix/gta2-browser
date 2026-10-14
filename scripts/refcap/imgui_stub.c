/* No-op dear_imgui.dll so upstream's decomp_main.exe can load. With the original
 * game's d3ddll.dll the ImGui debug overlay is never drawn, so these exports
 * are only resolved at load time. All are __cdecl; see imgui_stub.def. */
static char g_io[4096];
int imgui_stub_zero(void) { return 0; }
void *imgui_stub_io(void) { return g_io; }
