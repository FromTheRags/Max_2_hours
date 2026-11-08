#ifndef UI_H
#define UI_H

#include <SDL3/SDL.h>

bool ui_init();
void ui_cleanup();
void ui_actualiser();
void ui_set_hover(bool hover_minus, bool hover_plus);
SDL_Window *ui_get_window();
SDL_Renderer *ui_get_renderer();

#endif // UI_H
