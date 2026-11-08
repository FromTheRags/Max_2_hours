#if defined(_MSC_VER) && (_MSC_VER >= 1900)
#include <cstdio>
extern "C" {
    FILE * __cdecl __iob_func(void)
    {
        return __acrt_iob_func(0);
    }
}
#endif

#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <string>

#if defined(_WIN32)
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#endif

#if defined(__APPLE__)
    #include <SDL/SDL.h>
    #include <SDL/SDL_ttf.h>
#elif defined(__linux__) || defined(__unix__)
    #if __has_include(<SDL/SDL.h>)
        #include <SDL/SDL.h>
    #else
        #include <SDL.h>
    #endif
    #if __has_include(<SDL/SDL_ttf.h>)
        #include <SDL/SDL_ttf.h>
    #else
        #include <SDL_ttf.h>
    #endif
#else
    #if __has_include(<SDL/SDL.h>)
        #include <SDL/SDL.h>
    #else
        #include <SDL.h>
    #endif
    #if __has_include(<SDL/SDL_ttf.h>)
        #include <SDL/SDL_ttf.h>
    #else
        #include <SDL_ttf.h>
    #endif
#endif

// Dimensions de la fenêtre
const int FENETRE_L = 245;
const int FENETRE_H = 40;

// Variables globales d'état
static SDL_Surface *g_ecran = NULL;
static SDL_Surface *g_play_surface = NULL;
static TTF_Font *g_police = NULL;
static SDL_TimerID g_timer = NULL;

static int g_heures = 2;
static int g_minutes = 0;
static int g_secondes = 0;
static int g_playing = 1;
static bool g_only_one = true;

#if defined(_WIN32)
static HWND g_hwnd = NULL;
#endif

static const SDL_Color g_couleur_orange = {255, 123, 0, 0};

// Résolution portable des chemins d'accès aux assets
static std::string get_asset_path(const char *name)
{
    std::string candidate = std::string("assets/") + name;
    FILE *f = fopen(candidate.c_str(), "rb");
    if (f)
    {
        fclose(f);
        return candidate;
    }
    return std::string(name);
}

// Rafraîchissement de l'affichage
static void actualiser_affichage()
{
    if (!g_ecran) return;

    // Fond blanc
    SDL_FillRect(g_ecran, NULL, SDL_MapRGB(g_ecran->format, 255, 255, 255));

    if (g_playing)
    {
        if (g_police)
        {
            char temps[64] = {0};
            snprintf(temps, sizeof(temps), "Temps: %02d : %02d : %02d", g_heures, g_minutes, g_secondes);

            SDL_Surface *texte = TTF_RenderText_Blended(g_police, temps, g_couleur_orange);
            if (texte)
            {
                SDL_Rect positexte;
                positexte.x = (g_ecran->w - texte->w) / 2;
                positexte.y = (g_ecran->h - texte->h) / 2;
                SDL_BlitSurface(texte, NULL, g_ecran, &positexte);
                SDL_FreeSurface(texte);
            }
        }
    }
    else
    {
        // En pause : afficher l'icône lecture
        if (g_play_surface)
        {
            SDL_Rect position;
            position.x = (g_ecran->w - g_play_surface->w) / 2;
            position.y = (g_ecran->h - g_play_surface->h) / 2;
            SDL_BlitSurface(g_play_surface, NULL, g_ecran, &position);
        }
    }

    SDL_Flip(g_ecran);
}

// Callback appelé chaque seconde par le Timer SDL
static Uint32 timer_callback(Uint32 intervalle, void *parametre)
{
    (void)parametre;
    SDL_Event event;
    event.type = SDL_USEREVENT;
    SDL_PushEvent(&event);
    return intervalle;
}

// Mise à jour du décompte chaque seconde
static void tick_seconde()
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
            // Fin du compte à rebours : réinitialisation et arrêt du timer
            g_heures = 0;
            g_minutes = 0;
            g_secondes = 0;
            actualiser_affichage();
            if (g_timer)
            {
                SDL_RemoveTimer(g_timer);
                g_timer = NULL;
            }

            // Déclenchement de la mise en veille prolongée (comme dans le projet d'origine)
#if defined(_WIN32)
            system("shutdown /h");
#else
            system("systemctl hibernate || shutdown -h now");
#endif
            return;
        }
    }

    // Alerte automatique à moins de 5 minutes
    if (g_minutes <= 5 && g_heures <= 0 && g_only_one)
    {
#if defined(_WIN32)
        if (g_hwnd)
        {
            ShowWindow(g_hwnd, SW_RESTORE);
            RECT rect;
            SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, 0);
            SetWindowPos(g_hwnd, HWND_TOPMOST, (rect.right - 250) / 2, (rect.bottom - 68) / 2, 250, 68, SWP_SHOWWINDOW);
        }
#endif
        g_only_one = false;
    }

    actualiser_affichage();
}

// Bascule Lecture / Pause
static void basculer_pause()
{
    if (g_playing == 1)
    {
        // Passage en pause
        if (g_timer)
        {
            SDL_RemoveTimer(g_timer);
            g_timer = NULL;
        }
        g_playing = 0;
        actualiser_affichage();
    }
    else
    {
        // Reprise du décompte
        g_playing = 1;
        actualiser_affichage();
        g_timer = SDL_AddTimer(1000, timer_callback, NULL);
    }
}

int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;

    // Initialisation de SDL Vidéo et Timer
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0)
    {
        fprintf(stderr, "Erreur initialisation SDL: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }
    atexit(SDL_Quit);

    // Initialisation de SDL_ttf
    if (TTF_Init() < 0)
    {
        fprintf(stderr, "Erreur initialisation SDL_ttf: %s\n", TTF_GetError());
        return EXIT_FAILURE;
    }

    // Chargement de l'icône de la fenêtre
    std::string icon_path = get_asset_path("app_icon.bmp");
    SDL_Surface *icon = SDL_LoadBMP(icon_path.c_str());
    if (icon)
    {
        SDL_SetColorKey(icon, SDL_SRCCOLORKEY, SDL_MapRGB(icon->format, 255, 0, 255));
        SDL_WM_SetIcon(icon, NULL);
    }

    // Titre de la fenêtre
    SDL_WM_SetCaption("Protect & Perf - 2 Heures", NULL);

    // Création de la fenêtre
    g_ecran = SDL_SetVideoMode(FENETRE_L, FENETRE_H, 16, SDL_HWSURFACE | SDL_DOUBLEBUF);
    if (!g_ecran)
    {
        fprintf(stderr, "Impossible d'ouvrir la fenêtre SDL: %s\n", SDL_GetError());
        TTF_Quit();
        return EXIT_FAILURE;
    }

#if defined(_WIN32)
    g_hwnd = GetActiveWindow();
#endif

    // Chargement de la police libre (Patrick Hand sous licence SIL OFL)
    std::string font_path = get_asset_path("PatrickHand-Regular.ttf");
    g_police = TTF_OpenFont(font_path.c_str(), 32);
    if (!g_police)
    {
        fprintf(stderr, "Erreur chargement de la police '%s': %s\n", font_path.c_str(), TTF_GetError());
        TTF_Quit();
        return EXIT_FAILURE;
    }

    // Chargement de l'icône Play (libre de droits, BMP natif avec couleur clé)
    std::string play_path = get_asset_path("play.bmp");
    g_play_surface = SDL_LoadBMP(play_path.c_str());
    if (g_play_surface)
    {
        SDL_SetColorKey(g_play_surface, SDL_SRCCOLORKEY, SDL_MapRGB(g_play_surface->format, 255, 255, 255));
    }

    // Premier affichage et lancement du timer
    actualiser_affichage();
    g_timer = SDL_AddTimer(1000, timer_callback, NULL);

    int continuer = 1;
    SDL_Event event;

    while (continuer)
    {
        if (SDL_WaitEvent(&event))
        {
            switch (event.type)
            {
            case SDL_QUIT:
                continuer = 0;
                break;

            case SDL_USEREVENT:
                tick_seconde();
                break;

            case SDL_MOUSEBUTTONUP:
                basculer_pause();
                break;

            case SDL_ACTIVEEVENT:
                if ((event.active.state & SDL_APPACTIVE) == SDL_APPACTIVE)
                {
                    // Si réduite et moins de 5 min restantes, restaurer
                    if (event.active.gain == 0 && g_minutes <= 5 && g_heures <= 0 && g_playing == 1)
                    {
#if defined(_WIN32)
                        if (g_hwnd) ShowWindow(g_hwnd, SW_RESTORE);
#endif
                    }
                }
                break;

            default:
                break;
            }
        }
    }

    // Nettoyage des ressources
    if (g_timer)
    {
        SDL_RemoveTimer(g_timer);
        g_timer = NULL;
    }
    if (g_play_surface)
    {
        SDL_FreeSurface(g_play_surface);
        g_play_surface = NULL;
    }
    if (icon)
    {
        SDL_FreeSurface(icon);
    }
    if (g_police)
    {
        TTF_CloseFont(g_police);
        g_police = NULL;
    }

    TTF_Quit();
    return EXIT_SUCCESS;
}

#if defined(_WIN32)
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    (void)hInstance;
    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nCmdShow;
    return main(__argc, __argv);
}
#endif

