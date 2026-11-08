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
    #include <shellapi.h>
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

// Titre court et dimensions adaptées pour une visibilité parfaite du titre
const char *TITRE_APPLICATION = "Protect & Perf";
const int FENETRE_L = 360;
const int FENETRE_H = 60;

// Variables globales d'état
static SDL_Surface *g_ecran = NULL;
static SDL_Surface *g_play_surface = NULL;
static SDL_Surface *g_icon_normal = NULL;
static SDL_Surface *g_icon_pause = NULL;
static TTF_Font *g_police = NULL;
static SDL_TimerID g_timer = NULL;

static int g_heures = 2;
static int g_minutes = 0;
static int g_secondes = 0;
static int g_playing = 1;
static bool g_only_one = true;
static Uint32 g_last_restore_time = 0;

static const SDL_Color g_couleur_orange = {255, 123, 0, 0};

// Déclarations préalables
static void basculer_pause();
static void actualiser_affichage();
static void actualiser_tray();

#if defined(_WIN32)
#define WM_TRAYICON (WM_USER + 1)
#define ID_TRAY_RESTAURER 1001
#define ID_TRAY_PAUSE     1002
#define ID_TRAY_QUITTER   1003

static HWND g_hwnd = NULL;
static NOTIFYICONDATAA g_nid;
static bool g_in_tray = false;
static WNDPROC g_old_wndproc = NULL;
static HICON g_hicon_normal = NULL;
static HICON g_hicon_pause = NULL;

static void restaurer_depuis_tray();

// Procédure de sous-classement pour intercepter les clics sur l'icône de la barre des tâches
static LRESULT CALLBACK TraySubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == WM_TRAYICON)
    {
        if (lParam == WM_LBUTTONDBLCLK || lParam == WM_LBUTTONUP)
        {
            restaurer_depuis_tray();
            return 0;
        }
        else if (lParam == WM_RBUTTONUP)
        {
            POINT pt;
            GetCursorPos(&pt);
            HMENU hMenu = CreatePopupMenu();
            AppendMenuA(hMenu, MF_STRING, ID_TRAY_RESTAURER, "Afficher la fenetre");
            AppendMenuA(hMenu, MF_STRING, ID_TRAY_PAUSE, (g_playing == 1) ? "Pause" : "Reprendre");
            AppendMenuA(hMenu, MF_SEPARATOR, 0, NULL);
            AppendMenuA(hMenu, MF_STRING, ID_TRAY_QUITTER, "Quitter");

            SetForegroundWindow(hwnd);
            int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, 0, hwnd, NULL);
            DestroyMenu(hMenu);

            if (cmd == ID_TRAY_RESTAURER)
            {
                restaurer_depuis_tray();
            }
            else if (cmd == ID_TRAY_PAUSE)
            {
                basculer_pause();
            }
            else if (cmd == ID_TRAY_QUITTER)
            {
                SDL_Event ev;
                ev.type = SDL_QUIT;
                SDL_PushEvent(&ev);
            }
            return 0;
        }
    }
    return CallWindowProc(g_old_wndproc, hwnd, uMsg, wParam, lParam);
}

// Minimiser vers le System Tray
static void minimiser_vers_tray()
{
    if (g_hwnd && !g_in_tray)
    {
        ShowWindow(g_hwnd, SW_HIDE);
        g_in_tray = true;

        ZeroMemory(&g_nid, sizeof(g_nid));
        g_nid.cbSize = sizeof(g_nid);
        g_nid.hWnd = g_hwnd;
        g_nid.uID = 1;
        g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
        g_nid.uCallbackMessage = WM_TRAYICON;
        g_nid.hIcon = (g_playing == 1) ? g_hicon_normal : g_hicon_pause;
        if (!g_nid.hIcon)
        {
            g_nid.hIcon = (HICON)GetClassLongPtr(g_hwnd, GCLP_HICON);
        }
        snprintf(g_nid.szTip, sizeof(g_nid.szTip), "%s - %02d : %02d : %02d", TITRE_APPLICATION, g_heures, g_minutes, g_secondes);
        Shell_NotifyIconA(NIM_ADD, &g_nid);
    }
}

// Restauration en un seul clic depuis le System Tray
static void restaurer_depuis_tray()
{
    if (g_hwnd && g_in_tray)
    {
        g_last_restore_time = SDL_GetTicks();
        Shell_NotifyIconA(NIM_DELETE, &g_nid);
        g_in_tray = false;
        ShowWindow(g_hwnd, SW_SHOW);
        ShowWindow(g_hwnd, SW_RESTORE);
        SetForegroundWindow(g_hwnd);
    }
}

// Mise à jour de l'icône et de l'infobulle du System Tray (normale vs pause)
static void actualiser_tray()
{
    if (g_in_tray)
    {
        g_nid.hIcon = (g_playing == 1) ? g_hicon_normal : g_hicon_pause;
        if (!g_nid.hIcon)
        {
            g_nid.hIcon = (HICON)GetClassLongPtr(g_hwnd, GCLP_HICON);
        }
        if (g_playing == 1)
        {
            snprintf(g_nid.szTip, sizeof(g_nid.szTip), "%s - %02d : %02d : %02d", TITRE_APPLICATION, g_heures, g_minutes, g_secondes);
        }
        else
        {
            snprintf(g_nid.szTip, sizeof(g_nid.szTip), "%s [PAUSE] - %02d : %02d : %02d", TITRE_APPLICATION, g_heures, g_minutes, g_secondes);
        }
        g_nid.uFlags = NIF_ICON | NIF_TIP;
        Shell_NotifyIconA(NIM_MODIFY, &g_nid);
    }
}
#endif

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
        // En pause : afficher l'icône lecture au centre
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
            // Fin du compte à rebours : arrêt du timer
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
            restaurer_depuis_tray();
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
#if defined(_WIN32)
        restaurer_depuis_tray();
        if (g_hwnd)
        {
            ShowWindow(g_hwnd, SW_RESTORE);
            RECT rect;
            SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, 0);
            SetWindowPos(g_hwnd, HWND_TOPMOST, (rect.right - FENETRE_L) / 2, (rect.bottom - (FENETRE_H + 28)) / 2, FENETRE_L, FENETRE_H + 28, SWP_SHOWWINDOW);
        }
#endif
        g_only_one = false;
    }

    actualiser_affichage();

#if defined(_WIN32)
    actualiser_tray();
#endif
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
        if (g_icon_pause)
        {
            SDL_WM_SetIcon(g_icon_pause, NULL);
        }
    }
    else
    {
        // Reprise du décompte
        g_playing = 1;
        if (g_icon_normal)
        {
            SDL_WM_SetIcon(g_icon_normal, NULL);
        }
        g_timer = SDL_AddTimer(1000, timer_callback, NULL);
    }
    actualiser_affichage();

#if defined(_WIN32)
    actualiser_tray();
#endif
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

    // Chargement de l'icône normale
    std::string icon_path = get_asset_path("app_icon.bmp");
    g_icon_normal = SDL_LoadBMP(icon_path.c_str());
    if (g_icon_normal)
    {
        SDL_SetColorKey(g_icon_normal, SDL_SRCCOLORKEY, SDL_MapRGB(g_icon_normal->format, 255, 0, 255));
        SDL_WM_SetIcon(g_icon_normal, NULL);
    }

    // Chargement de l'icône pause
    std::string icon_pause_path = get_asset_path("app_icon_pause.bmp");
    g_icon_pause = SDL_LoadBMP(icon_pause_path.c_str());
    if (g_icon_pause)
    {
        SDL_SetColorKey(g_icon_pause, SDL_SRCCOLORKEY, SDL_MapRGB(g_icon_pause->format, 255, 0, 255));
    }

    // Titre court de la fenêtre
    SDL_WM_SetCaption(TITRE_APPLICATION, NULL);

    // Création de la fenêtre
    g_ecran = SDL_SetVideoMode(FENETRE_L, FENETRE_H, 16, SDL_HWSURFACE | SDL_DOUBLEBUF);
    if (!g_ecran)
    {
        fprintf(stderr, "Impossible d'ouvrir la fenetre SDL: %s\n", SDL_GetError());
        TTF_Quit();
        return EXIT_FAILURE;
    }

#if defined(_WIN32)
    g_hwnd = GetActiveWindow();
    if (g_hwnd)
    {
        g_old_wndproc = (WNDPROC)SetWindowLongPtrA(g_hwnd, GWLP_WNDPROC, (LONG_PTR)TraySubclassProc);
    }
    // Chargement des icônes HICON pour la zone de notification Windows
    std::string ico_normal_path = get_asset_path("clock.ico");
    g_hicon_normal = (HICON)LoadImageA(NULL, ico_normal_path.c_str(), IMAGE_ICON, 16, 16, LR_LOADFROMFILE);
    std::string ico_pause_path = get_asset_path("clock_pause.ico");
    g_hicon_pause = (HICON)LoadImageA(NULL, ico_pause_path.c_str(), IMAGE_ICON, 16, 16, LR_LOADFROMFILE);
#endif

    // Chargement de la police libre (Patrick Hand sous licence SIL OFL)
    std::string font_path = get_asset_path("PatrickHand-Regular.ttf");
    g_police = TTF_OpenFont(font_path.c_str(), 36);
    if (!g_police)
    {
        fprintf(stderr, "Erreur chargement police '%s': %s\n", font_path.c_str(), TTF_GetError());
        TTF_Quit();
        return EXIT_FAILURE;
    }

    // Chargement de l'icône Play
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
                    // Si minimisée : masquer et basculer dans le systray
                    // (ignore les événements transitoires dans les 500ms d'une restauration)
                    if (event.active.gain == 0 && g_playing == 1)
                    {
#if defined(_WIN32)
                        if (SDL_GetTicks() - g_last_restore_time > 500)
                        {
                            minimiser_vers_tray();
                        }
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
#if defined(_WIN32)
    restaurer_depuis_tray();
    if (g_hwnd && g_old_wndproc)
    {
        SetWindowLongPtrA(g_hwnd, GWLP_WNDPROC, (LONG_PTR)g_old_wndproc);
    }
    if (g_hicon_normal) DestroyIcon(g_hicon_normal);
    if (g_hicon_pause) DestroyIcon(g_hicon_pause);
#endif

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
    if (g_icon_normal)
    {
        SDL_FreeSurface(g_icon_normal);
        g_icon_normal = NULL;
    }
    if (g_icon_pause)
    {
        SDL_FreeSurface(g_icon_pause);
        g_icon_pause = NULL;
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
