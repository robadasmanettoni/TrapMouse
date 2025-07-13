#pragma once
#include <windows.h>

// Installa l’hook global per bloccare Alt+Tab, Win, Ctrl+Esc, Alt+F4...
bool InstallKeyBlocker();

// Rimuove l’hook
void UninstallKeyBlocker();
