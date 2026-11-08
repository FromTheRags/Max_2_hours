#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <string>

#if defined(_WIN32)
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
    #undef min
    #undef max
#elif defined(__APPLE__)
    #include <mach-o/dyld.h>
    #include <unistd.h>
#else
    #include <unistd.h>
#endif

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_ttf/SDL_ttf.h>

// Dimensions de la fenêtre
const int FENETRE_L = 245;
const int FENETRE_H = 40;

// Variables globales d'état
static SDL_Window *g_window = NULL;
static SDL_Renderer *g_renderer = NULL;
static SDL_Texture *g_play_texture = NULL;
static int g_play_w = 0;
static int g_play_h = 0;
static TTF_Font *g_police = NULL;
static SDL_TimerID g_timer = 0;

static int g_heures = 2;
static int g_minutes = 0;
static int g_secondes = 0;
static int g_playing = 1;
static bool g_only_one = true;

static const SDL_Color g_couleur_orange = {255, 123, 0, 255};

// Détermination portable du répertoire de l'exécutable
static std::string get_executable_dir()
{
#if defined(_WIN32)
    char buffer[MAX_PATH];
    DWORD len = GetModuleFileNameA(NULL, buffer, MAX_PATH);
    if (len > 0 && len < MAX_PATH)
    {
        std::string path(buffer, len);
        size_t pos = path.find_last_of("\\/");
        if (pos != std::string::npos)
        {
            return path.substr(0, pos + 1);
        }
    }
#elif defined(__APPLE__)
    char buffer[1024];
    uint32_t size = sizeof(buffer);
    if (_NSGetExecutablePath(buffer, &size) == 0)
    {
        std::string path(buffer);
        size_t pos = path.find_last_of('/');
        if (pos != std::string::npos)
        {
            return path.substr(0, pos + 1);
        }
    }
#else
    char buffer[1024];
    ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (len != -1)
    {
        buffer[len] = '\0';
        std::string path(buffer);
        size_t pos = path.find_last_of('/');
        if (pos != std::string::npos)
        {
            return path.substr(0, pos + 1);
        }
    }
#endif
    return "";
}

// Résolution portable des chemins d'accès aux assets
static std::string get_asset_path(const char *name)
{
    // 1. Relatif au répertoire de travail : assets/<name>
    std::string candidate = std::string("assets/") + name;
    FILE *f = fopen(candidate.c_str(), "rb");
    if (f)
    {
        fclose(f);
        return candidate;
    }

    // 2. À côté de l'exécutable : <exe_dir>/assets/<name>
    std::string exe_dir = get_executable_dir();
    if (!exe_dir.empty())
    {
        candidate = exe_dir + "assets/" + name;
        f = fopen(candidate.c_str(), "rb");
        if (f)
        {
            fclose(f);
            return candidate;
        }
    }

    // 3. Répertoire parent : ../assets/<name> (utile en mode debug / build)
    candidate = std::string("../assets/") + name;
    f = fopen(candidate.c_str(), "rb");
    if (f)
    {
        fclose(f);
        return candidate;
    }

    // 4. Racine courante
    candidate = std::string(name);
    f = fopen(candidate.c_str(), "rb");
    if (f)
    {
        fclose(f);
        return candidate;
    }

    return std::string("assets/") + name;
}

// Rafraîchissement de l'affichage avec le rendu SDL 3 accéléré
static void actualiser_affichage()
{
    if (!g_renderer) return;

    // Fond blanc
    SDL_SetRenderDrawColor(g_renderer, 255, 255, 255, 255);
    SDL_RenderClear(g_renderer);

    if (g_playing)
    {
        if (g_police)
        {
            char temps[64] = {0};
            snprintf(temps, sizeof(temps), "Temps: %02d : %02d : %02d", g_heures, g_minutes, g_secondes);

            SDL_Surface *texte_surf = TTF_RenderText_Blended(g_police, temps, 0, g_couleur_orange);
            if (texte_surf)
            {
                SDL_Texture *texte_tex = SDL_CreateTextureFromSurface(g_renderer, texte_surf);
                if (texte_tex)
                {
                    SDL_FRect dst;
                    dst.w = (float)texte_surf->w;
                    dst.h = (float)texte_surf->h;
                    dst.x = (FENETRE_L - dst.w) / 2.0f;
                    dst.y = (FENETRE_H - dst.h) / 2.0f;
                    SDL_RenderTexture(g_renderer, texte_tex, NULL, &dst);
                    SDL_DestroyTexture(texte_tex);
                }
                SDL_DestroySurface(texte_surf);
            }
        }
    }
    else
    {
        // En pause : afficher l'icône lecture au centre
        if (g_play_texture)
        {
            SDL_FRect dst;
            dst.w = (float)g_play_w;
            dst.h = (float)g_play_h;
            dst.x = (FENETRE_L - dst.w) / 2.0f;
            dst.y = (FENETRE_H - dst.h) / 2.0f;
            SDL_RenderTexture(g_renderer, g_play_texture, NULL, &dst);
        }
    }

    SDL_RenderPresent(g_renderer);
}

// Callback appelé chaque seconde par le Timer SDL 3
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
            // Fin du compte à rebours : arrêt du timer
            g_heures = 0;
            g_minutes = 0;
            g_secondes = 0;
            actualiser_affichage();
            if (g_timer != 0)
            {
                SDL_RemoveTimer(g_timer);
                g_timer = 0;
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

    // Alerte automatique à moins de 5 minutes restantes
    if (g_minutes <= 5 && g_heures <= 0 && g_only_one)
    {
        if (g_window)
        {
            SDL_RestoreWindow(g_window);
            SDL_SetWindowAlwaysOnTop(g_window, true);
        }
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
        if (g_timer != 0)
        {
            SDL_RemoveTimer(g_timer);
            g_timer = 0;
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

    // Initialisation de SDL 3
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        fprintf(stderr, "Erreur initialisation SDL 3: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }

    // Initialisation de SDL3_ttf
    if (!TTF_Init())
    {
        fprintf(stderr, "Erreur initialisation SDL3_ttf: %s\n", SDL_GetError());
        SDL_Quit();
        return EXIT_FAILURE;
    }

    // Création de la fenêtre et du renderer matériel SDL 3
    g_window = SDL_CreateWindow("Protect & Perf - 2 Heures", FENETRE_L, FENETRE_H, 0);
    if (!g_window)
    {
        fprintf(stderr, "Impossible de créer la fenêtre SDL 3: %s\n", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return EXIT_FAILURE;
    }

    g_renderer = SDL_CreateRenderer(g_window, NULL);
    if (!g_renderer)
    {
        fprintf(stderr, "Impossible de créer le renderer SDL 3: %s\n", SDL_GetError());
        SDL_DestroyWindow(g_window);
        TTF_Quit();
        SDL_Quit();
        return EXIT_FAILURE;
    }

    // Chargement de l'icône de la fenêtre
    std::string icon_path = get_asset_path("app_icon.bmp");
    SDL_Surface *icon = SDL_LoadBMP(icon_path.c_str());
    if (icon)
    {
        SDL_SetSurfaceColorKey(icon, true, SDL_MapSurfaceRGB(icon, 255, 0, 255));
        SDL_SetWindowIcon(g_window, icon);
        SDL_DestroySurface(icon);
    }

    // Chargement de la police libre (Patrick Hand sous licence SIL OFL)
    std::string font_path = get_asset_path("PatrickHand-Regular.ttf");
    g_police = TTF_OpenFont(font_path.c_str(), 32);
    if (!g_police)
    {
        fprintf(stderr, "Erreur chargement police '%s': %s\n", font_path.c_str(), SDL_GetError());
        SDL_DestroyRenderer(g_renderer);
        SDL_DestroyWindow(g_window);
        TTF_Quit();
        SDL_Quit();
        return EXIT_FAILURE;
    }

    // Chargement de l'icône Play
    std::string play_path = get_asset_path("play.bmp");
    SDL_Surface *play_surf = SDL_LoadBMP(play_path.c_str());
    if (play_surf)
    {
        SDL_SetSurfaceColorKey(play_surf, true, SDL_MapSurfaceRGB(play_surf, 255, 255, 255));
        g_play_w = play_surf->w;
        g_play_h = play_surf->h;
        g_play_texture = SDL_CreateTextureFromSurface(g_renderer, play_surf);
        SDL_DestroySurface(play_surf);
    }

    // Premier affichage et lancement du timer
    actualiser_affichage();
    g_timer = SDL_AddTimer(1000, timer_callback, NULL);

    bool continuer = true;
    SDL_Event event;

    while (continuer)
    {
        if (SDL_WaitEvent(&event))
        {
            switch (event.type)
            {
            case SDL_EVENT_QUIT:
                continuer = false;
                break;

            case SDL_EVENT_USER:
                tick_seconde();
                break;

            case SDL_EVENT_MOUSE_BUTTON_UP:
                basculer_pause();
                break;

            case SDL_EVENT_WINDOW_MINIMIZED:
                if (g_minutes <= 5 && g_heures <= 0 && g_playing == 1)
                {
                    SDL_RestoreWindow(g_window);
                }
                break;

            default:
                break;
            }
        }
    }

    // Nettoyage des ressources SDL 3
    if (g_timer != 0)
    {
        SDL_RemoveTimer(g_timer);
        g_timer = 0;
    }
    if (g_play_texture)
    {
        SDL_DestroyTexture(g_play_texture);
        g_play_texture = NULL;
    }
    if (g_police)
    {
        TTF_CloseFont(g_police);
        g_police = NULL;
    }
    if (g_renderer)
    {
        SDL_DestroyRenderer(g_renderer);
        g_renderer = NULL;
    }
    if (g_window)
    {
        SDL_DestroyWindow(g_window);
        g_window = NULL;
    }

    TTF_Quit();
    SDL_Quit();
    return EXIT_SUCCESS;
}
