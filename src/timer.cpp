#include "timer.h"
#include "common.h"
#include "ui.h"
#include "systray.h"
#include <cstdlib>

int g_heures = 2;
int g_minutes = 0;
int g_secondes = 0;
int g_playing = 1;
bool g_only_one = true;

static SDL_TimerID g_timer = NULL;

static Uint32 timer_callback(Uint32 interval, void *param)
{
    (void)param;
    SDL_Event event;
    event.type = SDL_USEREVENT;
    SDL_PushEvent(&event);
    return interval;
}

bool timer_init()
{
    g_timer = SDL_AddTimer(1000, timer_callback, NULL);
    return (g_timer != NULL);
}

void timer_cleanup()
{
    if (g_timer)
    {
        SDL_RemoveTimer(g_timer);
        g_timer = NULL;
    }
}

void timer_ajuster(int delta_secondes)
{
    int total = g_heures * 3600 + g_minutes * 60 + g_secondes;
    total += delta_secondes;

    // Minimum 1 minute (60s)
    if (total < 60)
    {
        total = 60;
    }
    // Maximum 24 heures (86400s)
    if (total > 86400)
    {
        total = 86400;
    }

    g_heures = total / 3600;
    g_minutes = (total % 3600) / 60;
    g_secondes = total % 60;

    if (total > 300)
    {
        g_only_one = true;
    }

    ui_actualiser();
    systray_actualiser();
}

void timer_basculer_pause()
{
    if (g_playing == 1)
    {
        if (g_timer)
        {
            SDL_RemoveTimer(g_timer);
            g_timer = NULL;
        }
        g_playing = 0;
        ui_set_icon_pause();
    }
    else
    {
        g_playing = 1;
        ui_set_icon_normal();
        g_timer = SDL_AddTimer(1000, timer_callback, NULL);
    }
    ui_actualiser();
    systray_actualiser();
}

void timer_tick()
{
    if (!g_playing) return;

    g_secondes--;
    if (g_secondes < 0)
    {
        if (g_minutes >= 1)
        {
            g_secondes = 59;
            g_minutes--;
        }
        else if (g_heures >= 1)
        {
            g_secondes = 59;
            g_minutes = 59;
            g_heures--;
        }
        else
        {
            // Fin du compte à rebours
            g_heures = 0;
            g_minutes = 0;
            g_secondes = 0;
            ui_actualiser();
            timer_cleanup();

            // Déclenchement de la mise en veille prolongée
#if defined(_WIN32)
            systray_restaurer();
            system("shutdown /h");
#else
            system("systemctl hibernate || shutdown -h now");
#endif
            return;
        }
    }

    // Alerte automatique à moins de 5 minutes restantes
    if (g_minutes <= 5 && g_heures <= 0 && g_only_one)
    {
        systray_restaurer();
#if defined(_WIN32)
        HWND hwnd = systray_get_hwnd();
        if (hwnd)
        {
            ShowWindow(hwnd, SW_RESTORE);
            RECT rect;
            SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, 0);
            SetWindowPos(hwnd, HWND_TOPMOST, (rect.right - FENETRE_L) / 2, (rect.bottom - (FENETRE_H + 28)) / 2, FENETRE_L, FENETRE_H + 28, SWP_SHOWWINDOW);
        }
#endif
        g_only_one = false;
    }

    ui_actualiser();
    systray_actualiser();
}
