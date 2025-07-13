#include "keyboard_hook.h"
#include <windows.h>
#include <winuser.h>

static HHOOK g_hKeyboardHook = NULL;

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)) {
        KBDLLHOOKSTRUCT* p = (KBDLLHOOKSTRUCT*)lParam;

        bool alt  = GetAsyncKeyState(VK_MENU)   & 0x8000;
        bool ctrl = GetAsyncKeyState(VK_CONTROL) & 0x8000;

        switch (p->vkCode) {
            case VK_TAB:    if (alt)  return 1; break;  // Alt+Tab
            case VK_ESCAPE: if (ctrl) return 1; break;  // Ctrl+Esc
            case VK_F4:     if (alt)  return 1; break;  // Alt+F4
            case VK_LWIN:   return 1;
            case VK_RWIN:   return 1;
        }
    }

    return CallNextHookEx(g_hKeyboardHook, nCode, wParam, lParam);
}
bool InstallKeyBlocker() {
    if (g_hKeyboardHook) return false;
    g_hKeyboardHook = SetWindowsHookEx(
        WH_KEYBOARD_LL,
        LowLevelKeyboardProc,
        GetModuleHandle(NULL),
        0
    );
    return g_hKeyboardHook != NULL;
}

void UninstallKeyBlocker() {
    if (g_hKeyboardHook) {
        UnhookWindowsHookEx(g_hKeyboardHook);
        g_hKeyboardHook = NULL;
    }
}
