#include "common.h"
#include "timer.h"
#include "ui.h"
#include "systray.h"
#include <SDL3/SDL_main.h>

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_Log("Impossible d'initialiser SDL: %s", SDL_GetError());
        return EXIT_FAILURE;
    }

    if (!ui_init())
    {
        SDL_Quit();
        return EXIT_FAILURE;
    }

    systray_init();
    ui_actualiser();
    timer_init();

    bool continuer = true;
    SDL_Event event;

    while (continuer)
    {
        if (SDL_WaitEvent(&event))
        {
            switch (event.type)
            {
            // Fermeture de l'application : gère le bouton [X] de la fenêtre, Alt+F4 et le signal Quit
            case SDL_EVENT_QUIT:
            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                continuer = false;
                break;

            case SDL_EVENT_USER:
                timer_tick();
                break;

            case SDL_EVENT_MOUSE_MOTION:
            {
                float mx = event.motion.x;
                float my = event.motion.y;
                bool hm = (mx >= BTN_MINUS_X && mx < BTN_MINUS_X + BTN_TAILLE && my >= BTN_Y && my < BTN_Y + BTN_TAILLE);
                bool hp = (mx >= BTN_PLUS_X && mx < BTN_PLUS_X + BTN_TAILLE && my >= BTN_Y && my < BTN_Y + BTN_TAILLE);
                ui_set_hover(hm, hp);
                break;
            }

            case SDL_EVENT_MOUSE_WHEEL:
            {
                SDL_Keymod mod = SDL_GetModState();
                if (event.wheel.y > 0)
                {
                    timer_ajuster((mod & SDL_KMOD_SHIFT) ? 300 : 60);
                }
                else if (event.wheel.y < 0)
                {
                    timer_ajuster((mod & SDL_KMOD_SHIFT) ? -300 : -60);
                }
                break;
            }

            case SDL_EVENT_MOUSE_BUTTON_UP:
            {
                float bx = event.button.x;
                float by = event.button.y;
                Uint8 btn = event.button.button;
                SDL_Keymod mod = SDL_GetModState();

                if (bx >= BTN_MINUS_X && bx < BTN_MINUS_X + BTN_TAILLE && by >= BTN_Y && by < BTN_Y + BTN_TAILLE)
                {
                    int delta = (btn == SDL_BUTTON_RIGHT || (mod & SDL_KMOD_SHIFT)) ? -60 : ((mod & SDL_KMOD_CTRL) ? -900 : -300);
                    timer_ajuster(delta);
                }
                else if (bx >= BTN_PLUS_X && bx < BTN_PLUS_X + BTN_TAILLE && by >= BTN_Y && by < BTN_Y + BTN_TAILLE)
                {
                    int delta = (btn == SDL_BUTTON_RIGHT || (mod & SDL_KMOD_SHIFT)) ? 60 : ((mod & SDL_KMOD_CTRL) ? 900 : 300);
                    timer_ajuster(delta);
                }
                else if (btn == SDL_BUTTON_LEFT)
                {
                    timer_basculer_pause();
                }
                break;
            }

            case SDL_EVENT_WINDOW_MINIMIZED:
                if (SDL_GetTicks() - systray_get_last_restore_time() > 500)
                {
                    SDL_HideWindow(ui_get_window());
                }
                break;

            case SDL_EVENT_WINDOW_RESTORED:
            case SDL_EVENT_WINDOW_SHOWN:
                break;

            default:
                break;
            }
        }
    }

    timer_cleanup();
    systray_cleanup();
    ui_cleanup();
    SDL_Quit();

    return EXIT_SUCCESS;
}
