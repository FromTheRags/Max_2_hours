#include "common.h"
#include "timer.h"
#include "ui.h"
#include "systray.h"
#include <cstdio>
#include <cstdlib>

#if defined(_WIN32)
#include <windows.h>
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    (void)hInstance;
    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nCmdShow;
    return main(__argc, __argv);
}
#endif

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0)
    {
        fprintf(stderr, "Impossible d'initialiser SDL: %s\n", SDL_GetError());
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

    int continuer = 1;
    SDL_Event event;

    while (continuer)
    {
        if (SDL_WaitEvent(&event))
        {
            switch (event.type)
            {
            // Fermeture de l'application : gère le bouton [X] de la fenêtre, Alt+F4 et le signal Quit
            case SDL_QUIT:
                continuer = 0;
                break;

            case SDL_USEREVENT:
                timer_tick();
                break;

            case SDL_MOUSEMOTION:
            {
                int mx = event.motion.x;
                int my = event.motion.y;
                bool hm = (mx >= BTN_MINUS_X && mx < BTN_MINUS_X + BTN_TAILLE && my >= BTN_Y && my < BTN_Y + BTN_TAILLE);
                bool hp = (mx >= BTN_PLUS_X && mx < BTN_PLUS_X + BTN_TAILLE && my >= BTN_Y && my < BTN_Y + BTN_TAILLE);
                ui_set_hover(hm, hp);
                break;
            }

            case SDL_MOUSEBUTTONUP:
            {
                int bx = event.button.x;
                int by = event.button.y;
                Uint8 btn = event.button.button;
                SDLMod mod = SDL_GetModState();

                if (btn == SDL_BUTTON_WHEELUP)
                {
                    timer_ajuster((mod & KMOD_SHIFT) ? 300 : 60);
                }
                else if (btn == SDL_BUTTON_WHEELDOWN)
                {
                    timer_ajuster((mod & KMOD_SHIFT) ? -300 : -60);
                }
                else if (bx >= BTN_MINUS_X && bx < BTN_MINUS_X + BTN_TAILLE && by >= BTN_Y && by < BTN_Y + BTN_TAILLE)
                {
                    int delta = (btn == SDL_BUTTON_RIGHT || (mod & KMOD_SHIFT)) ? -60 : ((mod & KMOD_CTRL) ? -900 : -300);
                    timer_ajuster(delta);
                }
                else if (bx >= BTN_PLUS_X && bx < BTN_PLUS_X + BTN_TAILLE && by >= BTN_Y && by < BTN_Y + BTN_TAILLE)
                {
                    int delta = (btn == SDL_BUTTON_RIGHT || (mod & KMOD_SHIFT)) ? 60 : ((mod & KMOD_CTRL) ? 900 : 300);
                    timer_ajuster(delta);
                }
                else if (btn == SDL_BUTTON_LEFT)
                {
                    timer_basculer_pause();
                }
                break;
            }

            case SDL_ACTIVEEVENT:
                if ((event.active.state & SDL_APPACTIVE) == SDL_APPACTIVE)
                {
                    // Si minimisée : masquer et basculer dans le systray
                    if (event.active.gain == 0 && g_playing == 1)
                    {
                        if (SDL_GetTicks() - systray_get_last_restore_time() > 500)
                        {
                            systray_minimiser();
                        }
                    }
                }
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
