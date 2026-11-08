#include "timer.h"
#include "ui.h"
#include "systray.h"
#include <cstdlib>

int g_heures = 2;
int g_minutes = 0;
int g_secondes = 0;
int g_playing = 1;
bool g_only_one = true;

static SDL_TimerID g_timer = 0;

static Uint32 SDLCALL timer_callback(void *userdata, SDL_TimerID timerID, Uint32 interval)
{
    (void)userdata;
    (void)timerID;
    SDL_Event event;
    SDL_zero(event);
    event.type = SDL_EVENT_USER;
    SDL_PushEvent(&event);
    return interval;
}

bool timer_init()
{
    g_timer = SDL_AddTimer(1000, timer_callback, NULL);
    return (g_timer != 0);
}

void timer_cleanup()
{
    if (g_timer != 0)
    {
        SDL_RemoveTimer(g_timer);
        g_timer = 0;
    }
}

void timer_ajuster(int delta_secondes)
{
    int total = g_heures * 3600 + g_minutes * 60 + g_secondes;
    total += delta_secondes;

    // Minimum 1 minute (60s) pour éviter l'extinction accidentelle
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
        if (g_timer != 0)
        {
            SDL_RemoveTimer(g_timer);
            g_timer = 0;
        }
        g_playing = 0;
    }
    else
    {
        g_playing = 1;
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
            systray_restaurer_fenetre();
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
        systray_restaurer_fenetre();
        SDL_Window *win = ui_get_window();
        if (win)
        {
            SDL_SetWindowAlwaysOnTop(win, true);
        }
        g_only_one = false;
    }

    ui_actualiser();
    systray_actualiser();
}
