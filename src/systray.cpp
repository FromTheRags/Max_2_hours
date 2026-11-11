#include "systray.h"
#include "common.h"
#include "timer.h"
#include "ui.h"
#include <cstdio>

#if defined(_WIN32)

#define WM_TRAYICON (WM_USER + 1)
#define ID_TRAY_RESTAURER   1001
#define ID_TRAY_PAUSE       1002
#define ID_TRAY_AJOUTER_5M  1004
#define ID_TRAY_RETIRER_5M  1005
#define ID_TRAY_QUITTER     1003

static HWND g_hwnd = NULL;
static NOTIFYICONDATAA g_nid;
static bool g_in_tray = false;
static WNDPROC g_old_wndproc = NULL;
static HICON g_hicon_normal = NULL;
static HICON g_hicon_pause = NULL;
static Uint32 g_last_restore_time = 0;

HWND systray_get_hwnd() { return g_hwnd; }
Uint32 systray_get_last_restore_time() { return g_last_restore_time; }

void systray_minimiser()
{
    if (g_hwnd && !g_in_tray)
    {
        ShowWindow(g_hwnd, SW_HIDE);
        g_in_tray = true;

        memset(&g_nid, 0, sizeof(g_nid));
        g_nid.cbSize = sizeof(NOTIFYICONDATAA);
        g_nid.hWnd = g_hwnd;
        g_nid.uID = 1;
        g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
        g_nid.uCallbackMessage = WM_TRAYICON;
        g_nid.hIcon = (g_playing == 1) ? g_hicon_normal : g_hicon_pause;
        if (!g_nid.hIcon)
        {
            g_nid.hIcon = (HICON)GetClassLongPtr(g_hwnd, GCLP_HICON);
        }
        snprintf(g_nid.szTip, sizeof(g_nid.szTip), "%s %s - %02d : %02d : %02d", TITRE_APPLICATION, get_version_string().c_str(), g_heures, g_minutes, g_secondes);
        Shell_NotifyIconA(NIM_ADD, &g_nid);
    }
}

void systray_restaurer()
{
    if (g_hwnd && g_in_tray)
    {
        g_last_restore_time = SDL_GetTicks();
        Shell_NotifyIconA(NIM_DELETE, &g_nid);
        g_in_tray = false;
        ShowWindow(g_hwnd, SW_SHOW);
        ShowWindow(g_hwnd, SW_RESTORE);
        SetForegroundWindow(g_hwnd);
    }
}

static LRESULT CALLBACK TraySubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // Fermeture directe lorsque l'utilisateur clique sur le bouton [X] de la fenêtre
    if (uMsg == WM_CLOSE)
    {
        SDL_Event ev;
        ev.type = SDL_QUIT;
        SDL_PushEvent(&ev);
        return 0;
    }

    if (uMsg == WM_TRAYICON)
    {
        if (lParam == WM_LBUTTONDBLCLK || lParam == WM_LBUTTONUP)
        {
            systray_restaurer();
            return 0;
        }
        else if (lParam == WM_RBUTTONUP)
        {
            POINT pt;
            GetCursorPos(&pt);
            HMENU hMenu = CreatePopupMenu();
            AppendMenuA(hMenu, MF_STRING, ID_TRAY_RESTAURER, "Afficher la fenetre");
            AppendMenuA(hMenu, MF_STRING, ID_TRAY_PAUSE, (g_playing == 1) ? "Pause" : "Reprendre");
            AppendMenuA(hMenu, MF_SEPARATOR, 0, NULL);
            AppendMenuA(hMenu, MF_STRING, ID_TRAY_AJOUTER_5M, "+5 minutes");
            AppendMenuA(hMenu, MF_STRING, ID_TRAY_RETIRER_5M, "-5 minutes");
            AppendMenuA(hMenu, MF_SEPARATOR, 0, NULL);
            AppendMenuA(hMenu, MF_STRING, ID_TRAY_QUITTER, "Quitter");

            SetForegroundWindow(hwnd);
            int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, 0, hwnd, NULL);
            DestroyMenu(hMenu);

            if (cmd == ID_TRAY_RESTAURER)
            {
                systray_restaurer();
            }
            else if (cmd == ID_TRAY_PAUSE)
            {
                timer_basculer_pause();
            }
            else if (cmd == ID_TRAY_AJOUTER_5M)
            {
                timer_ajuster(300);
            }
            else if (cmd == ID_TRAY_RETIRER_5M)
            {
                timer_ajuster(-300);
            }
            else if (cmd == ID_TRAY_QUITTER)
            {
                SDL_Event ev;
                ev.type = SDL_QUIT;
                SDL_PushEvent(&ev);
            }
            return 0;
        }
    }
    return CallWindowProc(g_old_wndproc, hwnd, uMsg, wParam, lParam);
}

bool systray_init()
{
    g_hwnd = GetActiveWindow();
    if (g_hwnd)
    {
        g_old_wndproc = (WNDPROC)SetWindowLongPtrA(g_hwnd, GWLP_WNDPROC, (LONG_PTR)TraySubclassProc);
    }
    std::string ico_normal_path = get_asset_path("clock.ico");
    g_hicon_normal = (HICON)LoadImageA(NULL, ico_normal_path.c_str(), IMAGE_ICON, 16, 16, LR_LOADFROMFILE);
    std::string ico_pause_path = get_asset_path("clock_pause.ico");
    g_hicon_pause = (HICON)LoadImageA(NULL, ico_pause_path.c_str(), IMAGE_ICON, 16, 16, LR_LOADFROMFILE);

    return true;
}

void systray_cleanup()
{
    systray_restaurer();
    if (g_hwnd && g_old_wndproc)
    {
        SetWindowLongPtrA(g_hwnd, GWLP_WNDPROC, (LONG_PTR)g_old_wndproc);
        g_old_wndproc = NULL;
    }
    if (g_hicon_normal) { DestroyIcon(g_hicon_normal); g_hicon_normal = NULL; }
    if (g_hicon_pause) { DestroyIcon(g_hicon_pause); g_hicon_pause = NULL; }
}

void systray_actualiser()
{
    if (g_in_tray)
    {
        g_nid.hIcon = (g_playing == 1) ? g_hicon_normal : g_hicon_pause;
        if (!g_nid.hIcon)
        {
            g_nid.hIcon = (HICON)GetClassLongPtr(g_hwnd, GCLP_HICON);
        }
        if (g_playing == 1)
        {
            snprintf(g_nid.szTip, sizeof(g_nid.szTip), "%s %s - %02d : %02d : %02d", TITRE_APPLICATION, get_version_string().c_str(), g_heures, g_minutes, g_secondes);
        }
        else
        {
            snprintf(g_nid.szTip, sizeof(g_nid.szTip), "%s %s [PAUSE] - %02d : %02d : %02d", TITRE_APPLICATION, get_version_string().c_str(), g_heures, g_minutes, g_secondes);
        }
        g_nid.uFlags = NIF_ICON | NIF_TIP;
        Shell_NotifyIconA(NIM_MODIFY, &g_nid);
    }
}

#else

// Stubs pour Linux/macOS sous SDL 1.2
bool systray_init() { return true; }
void systray_cleanup() {}
void systray_actualiser() {}
void systray_minimiser() {}
void systray_restaurer() {}
Uint32 systray_get_last_restore_time() { return 0; }

#endif
