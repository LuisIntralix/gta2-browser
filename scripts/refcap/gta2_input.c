/* Scripted keyboard input for the original GTA2 running under Wine.
 *
 * Runs inside the same Wine prefix as gta2.exe and injects scan-code key events
 * with SendInput, so DirectInput sees them. No macOS accessibility permission
 * is needed.
 *
 * Usage: gta2_input.exe <window-title> <step>...
 *   wait:<ms>              sleep
 *   tap:<VK>[:<hold-ms>]   press and release (default hold 80 ms)
 *   down:<VK> / up:<VK>    hold or release
 *   text:<ascii>           tap each letter/digit
 *   restore:0              un-minimise and refocus the window
 * VK is a decimal or 0x-hex virtual-key code, or one of the names below.
 */
/* Built without an SDK (see build_gta2_input.sh), so declare what we use. */
typedef unsigned short WORD;
typedef unsigned long DWORD;
typedef void *HWND;
typedef __SIZE_TYPE__ size_t;
#define WINAPI __stdcall
#define NULL ((void *)0)
#define INPUT_KEYBOARD 1
#define KEYEVENTF_EXTENDEDKEY 0x1
#define KEYEVENTF_KEYUP 0x2
#define KEYEVENTF_SCANCODE 0x8
#define MAPVK_VK_TO_VSC 0
enum { VK_BACK = 0x08, VK_TAB = 0x09, VK_RETURN = 0x0D, VK_SHIFT = 0x10, VK_CONTROL = 0x11,
       VK_MENU = 0x12, VK_ESCAPE = 0x1B, VK_SPACE = 0x20, VK_LEFT = 0x25, VK_UP = 0x26,
       VK_RIGHT = 0x27, VK_DOWN = 0x28, VK_F1 = 0x70, VK_F12 = 0x7B };
typedef struct { WORD wVk, wScan; DWORD dwFlags, time; unsigned long dwExtraInfo; } KEYBDINPUT;
typedef struct { DWORD type; union { KEYBDINPUT ki; char pad[24]; }; } INPUT;
__declspec(dllimport) unsigned WINAPI SendInput(unsigned, INPUT *, int);
__declspec(dllimport) unsigned WINAPI MapVirtualKeyA(unsigned, unsigned);
__declspec(dllimport) HWND WINAPI FindWindowA(const char *, const char *);
__declspec(dllimport) int WINAPI SetForegroundWindow(HWND);
__declspec(dllimport) int WINAPI ShowWindow(HWND, int);
#define SW_RESTORE 9
__declspec(dllimport) void WINAPI Sleep(DWORD);
__declspec(dllimport) int printf(const char *, ...);
__declspec(dllimport) int fprintf(void *, const char *, ...);
__declspec(dllimport) int fflush(void *);
__declspec(dllimport) void *__iob_func(void);
__declspec(dllimport) unsigned long strtoul(const char *, char **, int);
__declspec(dllimport) char *strncpy(char *, const char *, size_t);
__declspec(dllimport) char *strchr(const char *, int);
__declspec(dllimport) int strcmp(const char *, const char *);
__declspec(dllimport) int _stricmp(const char *, const char *);
__declspec(dllimport) int toupper(int);
__declspec(dllimport) void exit(int);
__declspec(dllimport) int __getmainargs(int *, char ***, char ***, int, void *);
#define stdout ((char *)__iob_func() + 32)
#define stderr ((char *)__iob_func() + 64)

static const struct { const char *name; WORD vk; } kNames[] = {
    {"RETURN", VK_RETURN}, {"ESCAPE", VK_ESCAPE}, {"SPACE", VK_SPACE},
    {"UP", VK_UP}, {"DOWN", VK_DOWN}, {"LEFT", VK_LEFT}, {"RIGHT", VK_RIGHT},
    {"CTRL", VK_CONTROL}, {"SHIFT", VK_SHIFT}, {"ALT", VK_MENU}, {"TAB", VK_TAB},
    {"BACK", VK_BACK}, {"F1", VK_F1}, {"F12", VK_F12},
};

static WORD parse_vk(const char *s)
{
    for (size_t i = 0; i < sizeof(kNames) / sizeof(kNames[0]); i++)
        if (!_stricmp(s, kNames[i].name))
            return kNames[i].vk;
    if (s[0] && !s[1])
        return (WORD)toupper((unsigned char)s[0]);
    return (WORD)strtoul(s, NULL, 0);
}

static int is_extended(WORD vk)
{
    return vk == VK_UP || vk == VK_DOWN || vk == VK_LEFT || vk == VK_RIGHT;
}

static void send_key(WORD vk, int up)
{
    INPUT in = {0};
    in.type = INPUT_KEYBOARD;
    in.ki.wVk = vk;
    in.ki.wScan = (WORD)MapVirtualKeyA(vk, MAPVK_VK_TO_VSC);
    in.ki.dwFlags = KEYEVENTF_SCANCODE | (up ? KEYEVENTF_KEYUP : 0) |
                    (is_extended(vk) ? KEYEVENTF_EXTENDEDKEY : 0);
    SendInput(1, &in, sizeof(in));
}

static void tap(WORD vk, DWORD hold_ms)
{
    send_key(vk, 0);
    Sleep(hold_ms);
    send_key(vk, 1);
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "usage: %s <window-title> <step>...\n", argv[0]);
        return 2;
    }
    HWND hwnd = NULL;
    for (int i = 0; i < 100 && !hwnd; i++, Sleep(100))
        hwnd = FindWindowA(NULL, argv[1]);
    if (!hwnd) {
        fprintf(stderr, "window '%s' not found\n", argv[1]);
        return 1;
    }
    SetForegroundWindow(hwnd);
    Sleep(200);

    for (int i = 2; i < argc; i++) {
        char step[256];
        strncpy(step, argv[i], sizeof(step) - 1);
        step[sizeof(step) - 1] = 0;
        char *arg = strchr(step, ':');
        if (!arg) {
            fprintf(stderr, "bad step '%s'\n", argv[i]);
            return 2;
        }
        *arg++ = 0;
        if (!strcmp(step, "wait")) {
            Sleep(strtoul(arg, NULL, 0));
        } else if (!strcmp(step, "tap")) {
            char *hold = strchr(arg, ':');
            if (hold)
                *hold++ = 0;
            tap(parse_vk(arg), hold ? strtoul(hold, NULL, 0) : 80);
        } else if (!strcmp(step, "down")) {
            send_key(parse_vk(arg), 0);
        } else if (!strcmp(step, "up")) {
            send_key(parse_vk(arg), 1);
        } else if (!strcmp(step, "restore")) {
            ShowWindow(hwnd, SW_RESTORE);
            SetForegroundWindow(hwnd);
        } else if (!strcmp(step, "text")) {
            for (const char *c = arg; *c; c++) {
                tap((WORD)toupper((unsigned char)*c), 60);
                Sleep(60);
            }
        } else {
            fprintf(stderr, "unknown step '%s'\n", step);
            return 2;
        }
        printf("%s\n", argv[i]);
        fflush(stdout);
    }
    return 0;
}

void mainCRTStartup(void)
{
    int argc, si = 0;
    char **argv, **envp;
    __getmainargs(&argc, &argv, &envp, 0, &si);
    exit(main(argc, argv));
}
