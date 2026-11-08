#ifndef TIMER_H
#define TIMER_H

#if defined(__APPLE__)
    #include <SDL/SDL.h>
#else
    #include <SDL.h>
#endif

// Variables d'état accessibles en lecture
extern int g_heures;
extern int g_minutes;
extern int g_secondes;
extern int g_playing;
extern bool g_only_one;

// Initialisation et gestion du timer
bool timer_init();
void timer_cleanup();
void timer_tick();
void timer_ajuster(int delta_secondes);
void timer_basculer_pause();

#endif // TIMER_H
