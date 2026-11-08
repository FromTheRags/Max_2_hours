#ifndef SYSTRAY_H
#define SYSTRAY_H

#include <SDL3/SDL.h>

bool systray_init();
void systray_cleanup();
void systray_actualiser();
void systray_restaurer_fenetre();
Uint64 systray_get_last_restore_time();

#endif // SYSTRAY_H
