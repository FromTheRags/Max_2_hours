#include "systray.h"
#include "common.h"
#include "timer.h"
#include "ui.h"
#include <cstdio>

#if defined(_WIN32)
    #include <windows.h>
#endif

static SDL_Tray *g_tray = NULL;
static SDL_TrayEntry *g_tray_entry_pause = NULL;
static SDL_Surface *g_icon_normal = NULL;
static SDL_Surface *g_icon_pause = NULL;
static Uint64 g_last_restore_time = 0;

Uint64 systray_get_last_restore_time()
{
    return g_last_restore_time;
}

void systray_restaurer_fenetre()
{
    SDL_Window *win = ui_get_window();
    if (win)
    {
        g_last_restore_time = SDL_GetTicks();

        SDL_RestoreWindow(win);
        SDL_ShowWindow(win);
        SDL_RaiseWindow(win);

#if defined(_WIN32)
        HWND hwnd = (HWND)SDL_GetPointerProperty(SDL_GetWindowProperties(win), SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
        if (!hwnd) hwnd = GetActiveWindow();
        if (hwnd)
        {
            ShowWindow(hwnd, SW_RESTORE);
            SetForegroundWindow(hwnd);
        }
#endif
    }
}

static void SDLCALL on_tray_restaurer(void *userdata, SDL_TrayEntry *entry)
{
    (void)userdata;
    (void)entry;
    systray_restaurer_fenetre();
}

static void SDLCALL on_tray_pause(void *userdata, SDL_TrayEntry *entry)
{
    (void)userdata;
    (void)entry;
    timer_basculer_pause();
}

static void SDLCALL on_tray_ajouter_5m(void *userdata, SDL_TrayEntry *entry)
{
    (void)userdata;
    (void)entry;
    timer_ajuster(300);
}

static void SDLCALL on_tray_retirer_5m(void *userdata, SDL_TrayEntry *entry)
{
    (void)userdata;
    (void)entry;
    timer_ajuster(-300);
}

static void SDLCALL on_tray_quitter(void *userdata, SDL_TrayEntry *entry)
{
    (void)userdata;
    (void)entry;
    SDL_Event ev;
    SDL_zero(ev);
    ev.type = SDL_EVENT_QUIT;
    SDL_PushEvent(&ev);
}

bool systray_init()
{
    // Chargement de l'icône normale
    std::string icon_path = get_asset_path("app_icon.bmp");
    g_icon_normal = SDL_LoadBMP(icon_path.c_str());
    if (g_icon_normal)
    {
        SDL_SetSurfaceColorKey(g_icon_normal, true, SDL_MapSurfaceRGB(g_icon_normal, 255, 0, 255));
        SDL_Window *win = ui_get_window();
        if (win)
        {
            SDL_SetWindowIcon(win, g_icon_normal);
        }
    }

    // Chargement de l'icône pause
    std::string icon_pause_path = get_asset_path("app_icon_pause.bmp");
    g_icon_pause = SDL_LoadBMP(icon_pause_path.c_str());
    if (g_icon_pause)
    {
        SDL_SetSurfaceColorKey(g_icon_pause, true, SDL_MapSurfaceRGB(g_icon_pause, 255, 0, 255));
    }

    g_tray = SDL_CreateTray(g_icon_normal ? g_icon_normal : NULL, TITRE_APPLICATION);
    if (g_tray)
    {
        SDL_TrayMenu *menu = SDL_CreateTrayMenu(g_tray);
        if (menu)
        {
            SDL_TrayEntry *e_restore = SDL_InsertTrayEntryAt(menu, -1, "Afficher la fenetre", SDL_TRAYENTRY_BUTTON);
            if (e_restore) SDL_SetTrayEntryCallback(e_restore, on_tray_restaurer, NULL);

            g_tray_entry_pause = SDL_InsertTrayEntryAt(menu, -1, "Pause", SDL_TRAYENTRY_BUTTON);
            if (g_tray_entry_pause) SDL_SetTrayEntryCallback(g_tray_entry_pause, on_tray_pause, NULL);

            SDL_TrayEntry *e_add5 = SDL_InsertTrayEntryAt(menu, -1, "+5 minutes", SDL_TRAYENTRY_BUTTON);
            if (e_add5) SDL_SetTrayEntryCallback(e_add5, on_tray_ajouter_5m, NULL);

            SDL_TrayEntry *e_sub5 = SDL_InsertTrayEntryAt(menu, -1, "-5 minutes", SDL_TRAYENTRY_BUTTON);
            if (e_sub5) SDL_SetTrayEntryCallback(e_sub5, on_tray_retirer_5m, NULL);

            SDL_TrayEntry *e_quit = SDL_InsertTrayEntryAt(menu, -1, "Quitter", SDL_TRAYENTRY_BUTTON);
            if (e_quit) SDL_SetTrayEntryCallback(e_quit, on_tray_quitter, NULL);
        }
    }

    systray_actualiser();
    return true;
}

void systray_cleanup()
{
    if (g_tray)
    {
        SDL_DestroyTray(g_tray);
        g_tray = NULL;
    }
    if (g_icon_normal)
    {
        SDL_DestroySurface(g_icon_normal);
        g_icon_normal = NULL;
    }
    if (g_icon_pause)
    {
        SDL_DestroySurface(g_icon_pause);
        g_icon_pause = NULL;
    }
}

void systray_actualiser()
{
    if (!g_tray) return;

    SDL_Surface *active_icon = (g_playing == 1) ? g_icon_normal : g_icon_pause;
    if (active_icon)
    {
        SDL_SetTrayIcon(g_tray, active_icon);
        SDL_Window *win = ui_get_window();
        if (win)
        {
            SDL_SetWindowIcon(win, active_icon);
        }
    }

    char tooltip[128] = {0};
    if (g_playing == 1)
    {
        snprintf(tooltip, sizeof(tooltip), "%s - %02d : %02d : %02d", TITRE_APPLICATION, g_heures, g_minutes, g_secondes);
    }
    else
    {
        snprintf(tooltip, sizeof(tooltip), "%s [PAUSE] - %02d : %02d : %02d", TITRE_APPLICATION, g_heures, g_minutes, g_secondes);
    }
    SDL_SetTrayTooltip(g_tray, tooltip);

    if (g_tray_entry_pause)
    {
        SDL_SetTrayEntryLabel(g_tray_entry_pause, (g_playing == 1) ? "Pause" : "Reprendre");
    }
}
