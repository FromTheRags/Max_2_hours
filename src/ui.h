#ifndef UI_H
#define UI_H

#if defined(__APPLE__)
    #include <SDL/SDL.h>
#else
    #include <SDL.h>
#endif

bool ui_init();
void ui_cleanup();
void ui_actualiser();
void ui_set_hover(bool hover_minus, bool hover_plus);
void ui_set_icon_normal();
void ui_set_icon_pause();
SDL_Surface *ui_get_screen();

#endif // UI_H
