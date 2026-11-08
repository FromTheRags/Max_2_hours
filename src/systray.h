#ifndef SYSTRAY_H
#define SYSTRAY_H

#if defined(__APPLE__)
    #include <SDL/SDL.h>
#else
    #include <SDL.h>
#endif

#if defined(_WIN32)
    #include <windows.h>
    HWND systray_get_hwnd();
#endif

bool systray_init();
void systray_cleanup();
void systray_actualiser();
void systray_minimiser();
void systray_restaurer();
Uint32 systray_get_last_restore_time();

#endif // SYSTRAY_H
