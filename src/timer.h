#ifndef TIMER_H
#define TIMER_H

#include <SDL3/SDL.h>

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
