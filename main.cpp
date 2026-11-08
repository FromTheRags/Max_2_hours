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

// Dimensions de la fenêtre et titre court pour une visibilité parfaite
// dans la barre de titre Windows/Linux sans être masqué par les boutons système
const char *TITRE_APPLICATION = "Protect & Perf";
const int FENETRE_L = 380;
const int FENETRE_H = 60;

// Dimensions et position des boutons - et +
const int BTN_TAILLE = 32;
const int BTN_Y = (FENETRE_H - BTN_TAILLE) / 2;
const int BTN_MINUS_X = 12;
const int BTN_PLUS_X = FENETRE_L - 12 - BTN_TAILLE;

// Variables globales d'état
static SDL_Window *g_window = NULL;
static SDL_Renderer *g_renderer = NULL;
static SDL_Texture *g_play_texture = NULL;
static int g_play_w = 0;
static int g_play_h = 0;
static TTF_Font *g_police = NULL;
static SDL_TimerID g_timer = 0;

static SDL_Tray *g_tray = NULL;
static SDL_TrayEntry *g_tray_entry_pause = NULL;
static SDL_Surface *g_icon_normal = NULL;
static SDL_Surface *g_icon_pause = NULL;

static int g_heures = 2;
static int g_minutes = 0;
static int g_secondes = 0;
static int g_playing = 1;
static bool g_only_one = true;
static bool g_is_minimized = false;
static Uint64 g_last_restore_time = 0;
static bool g_hover_minus = false;
static bool g_hover_plus = false;

static const SDL_Color g_couleur_orange = {255, 123, 0, 255};

// Déclarations préalables
static void basculer_pause();
static void restaurer_fenetre();
static void actualiser_tray_state();
static void actualiser_affichage();
static void ajuster_temps(int delta_secondes);

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

    // 3. Répertoire parent : ../assets/<name> (utile en build/debug)
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

// Ajustement de la durée restante
static void ajuster_temps(int delta_secondes)
{
    int total = g_heures * 3600 + g_minutes * 60 + g_secondes;
    total += delta_secondes;

    // Minimum 1 minute (60s) pour éviter l'extinction immédiate
    if (total < 60)
    {
        total = 60;
    }
    // Maximum 24 heures (86400s)
    if (total > 86400)
    {
        total = 86400;
    }

    g_heures = total / 3600;
    g_minutes = (total % 3600) / 60;
    g_secondes = total % 60;

    // Réinitialisation de l'alerte des 5 minutes si on repasse au-dessus
    if (total > 300)
    {
        g_only_one = true;
    }

    actualiser_affichage();
    actualiser_tray_state();
}

// Dessin vectoriel épuré des boutons - et + avec le renderer SDL 3
static void dessiner_bouton(SDL_Renderer *renderer, float x, float y, float taille, bool est_plus, bool hover)
{
    SDL_FRect r_bg = { x, y, taille, taille };

    // Remplissage fond
    if (hover)
    {
        SDL_SetRenderDrawColor(renderer, 255, 123, 0, 255);
    }
    else
    {
        SDL_SetRenderDrawColor(renderer, 246, 246, 248, 255);
    }
    SDL_RenderFillRect(renderer, &r_bg);

    // Bordure 1px
    if (hover)
    {
        SDL_SetRenderDrawColor(renderer, 220, 95, 0, 255);
    }
    else
    {
        SDL_SetRenderDrawColor(renderer, 215, 215, 220, 255);
    }
    SDL_RenderRect(renderer, &r_bg);

    // Couleur du symbole
    if (hover)
    {
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    }
    else
    {
        SDL_SetRenderDrawColor(renderer, 255, 123, 0, 255);
    }

    // Barre horizontale (- ou +)
    float barre_w = 14.0f;
    float barre_h = 3.0f;
    SDL_FRect r_h = {
        x + (taille - barre_w) / 2.0f,
        y + (taille - barre_h) / 2.0f,
        barre_w,
        barre_h
    };
    SDL_RenderFillRect(renderer, &r_h);

    // Barre verticale pour +
    if (est_plus)
    {
        SDL_FRect r_v = {
            x + (taille - barre_h) / 2.0f,
            y + (taille - barre_w) / 2.0f,
            barre_h,
            barre_w
        };
        SDL_RenderFillRect(renderer, &r_v);
    }
}

// Rafraîchissement de l'affichage avec le rendu SDL 3 accéléré
static void actualiser_affichage()
{
    if (!g_renderer) return;

    // Fond blanc
    SDL_SetRenderDrawColor(g_renderer, 255, 255, 255, 255);
    SDL_RenderClear(g_renderer);

    // Dessin des boutons - et +
    dessiner_bouton(g_renderer, (float)BTN_MINUS_X, (float)BTN_Y, (float)BTN_TAILLE, false, g_hover_minus);
    dessiner_bouton(g_renderer, (float)BTN_PLUS_X, (float)BTN_Y, (float)BTN_TAILLE, true, g_hover_plus);

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
        // En pause : afficher l'icône de reprise et le temps actuel
        if (g_police)
        {
            char temps[64] = {0};
            snprintf(temps, sizeof(temps), "%02d : %02d : %02d", g_heures, g_minutes, g_secondes);
            SDL_Surface *texte_surf = TTF_RenderText_Blended(g_police, temps, 0, g_couleur_orange);

            float play_w = g_play_texture ? (float)g_play_w : 0.0f;
            float play_h = g_play_texture ? (float)g_play_h : 0.0f;
            float txt_w = texte_surf ? (float)texte_surf->w : 0.0f;
            float txt_h = texte_surf ? (float)texte_surf->h : 0.0f;
            float espacement = 12.0f;
            float total_w = play_w + (play_w > 0.0f && txt_w > 0.0f ? espacement : 0.0f) + txt_w;
            float start_x = (FENETRE_L - total_w) / 2.0f;

            if (g_play_texture)
            {
                SDL_FRect pos_play;
                pos_play.x = start_x;
                pos_play.y = (FENETRE_H - play_h) / 2.0f;
                pos_play.w = play_w;
                pos_play.h = play_h;
                SDL_RenderTexture(g_renderer, g_play_texture, NULL, &pos_play);
            }

            if (texte_surf)
            {
                SDL_Texture *texte_tex = SDL_CreateTextureFromSurface(g_renderer, texte_surf);
                if (texte_tex)
                {
                    SDL_FRect pos_txt;
                    pos_txt.x = start_x + (play_w > 0.0f ? play_w + espacement : 0.0f);
                    pos_txt.y = (FENETRE_H - txt_h) / 2.0f;
                    pos_txt.w = txt_w;
                    pos_txt.h = txt_h;
                    SDL_RenderTexture(g_renderer, texte_tex, NULL, &pos_txt);
                    SDL_DestroyTexture(texte_tex);
                }
                SDL_DestroySurface(texte_surf);
            }
        }
    }

    SDL_RenderPresent(g_renderer);
}

// Mise à jour de l'icône et de l'infobulle du System Tray (normale vs pause)
static void actualiser_tray_state()
{
    if (!g_tray) return;

    // 1. Changement d'icône dans la zone de notification et la fenêtre
    SDL_Surface *active_icon = (g_playing == 1) ? g_icon_normal : g_icon_pause;
    if (active_icon)
    {
        SDL_SetTrayIcon(g_tray, active_icon);
        if (g_window)
        {
            SDL_SetWindowIcon(g_window, active_icon);
        }
    }

    // 2. Infobulle dynamique
    char tooltip[128] = {0};
    if (g_playing == 1)
    {
        snprintf(tooltip, sizeof(tooltip), "Protect & Perf - %02d : %02d : %02d", g_heures, g_minutes, g_secondes);
    }
    else
    {
        snprintf(tooltip, sizeof(tooltip), "Protect & Perf [PAUSE] - %02d : %02d : %02d", g_heures, g_minutes, g_secondes);
    }
    SDL_SetTrayTooltip(g_tray, tooltip);

    // 3. Libellé du menu contextuel
    if (g_tray_entry_pause)
    {
        SDL_SetTrayEntryLabel(g_tray_entry_pause, (g_playing == 1) ? "Pause" : "Reprendre");
    }
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

// Restauration garantie en un seul clic de la fenêtre depuis le Systray
static void restaurer_fenetre()
{
    if (g_window)
    {
        g_last_restore_time = SDL_GetTicks();
        g_is_minimized = false;

        // Ordre précis : d'abord dés-minimiser, puis afficher et remonter la fenêtre
        SDL_RestoreWindow(g_window);
        SDL_ShowWindow(g_window);
        SDL_RaiseWindow(g_window);

#if defined(_WIN32)
        HWND hwnd = (HWND)SDL_GetPointerProperty(SDL_GetWindowProperties(g_window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
        if (!hwnd) hwnd = GetActiveWindow();
        if (hwnd)
        {
            ShowWindow(hwnd, SW_RESTORE);
            SetForegroundWindow(hwnd);
        }
#endif
    }
}

// Callbacks pour le menu du System Tray
static void SDLCALL on_tray_restaurer(void *userdata, SDL_TrayEntry *entry)
{
    (void)userdata;
    (void)entry;
    restaurer_fenetre();
}

static void SDLCALL on_tray_pause(void *userdata, SDL_TrayEntry *entry)
{
    (void)userdata;
    (void)entry;
    basculer_pause();
}

static void SDLCALL on_tray_ajouter_5m(void *userdata, SDL_TrayEntry *entry)
{
    (void)userdata;
    (void)entry;
    ajuster_temps(300);
}

static void SDLCALL on_tray_retirer_5m(void *userdata, SDL_TrayEntry *entry)
{
    (void)userdata;
    (void)entry;
    ajuster_temps(-300);
}

static void SDLCALL on_tray_quitter(void *userdata, SDL_TrayEntry *entry)
{
    (void)userdata;
    (void)entry;
    SDL_Event ev;
    SDL_zero(ev);
    ev.type = SDL_EVENT_QUIT;
    SDL_PushEvent(&ev);
}

// Initialisation du System Tray natif cross-platform (Windows & Linux)
static void init_systray()
{
    // Chargement de l'icône normale
    std::string icon_path = get_asset_path("app_icon.bmp");
    g_icon_normal = SDL_LoadBMP(icon_path.c_str());
    if (g_icon_normal)
    {
        SDL_SetSurfaceColorKey(g_icon_normal, true, SDL_MapSurfaceRGB(g_icon_normal, 255, 0, 255));
    }

    // Chargement de l'icône pause
    std::string icon_pause_path = get_asset_path("app_icon_pause.bmp");
    g_icon_pause = SDL_LoadBMP(icon_pause_path.c_str());
    if (g_icon_pause)
    {
        SDL_SetSurfaceColorKey(g_icon_pause, true, SDL_MapSurfaceRGB(g_icon_pause, 255, 0, 255));
    }

    g_tray = SDL_CreateTray(g_icon_normal ? g_icon_normal : NULL, TITRE_APPLICATION);
    if (g_tray)
    {
        SDL_TrayMenu *menu = SDL_CreateTrayMenu(g_tray);
        if (menu)
        {
            SDL_TrayEntry *e_restore = SDL_InsertTrayEntryAt(menu, -1, "Afficher la fenetre", SDL_TRAYENTRY_BUTTON);
            if (e_restore) SDL_SetTrayEntryCallback(e_restore, on_tray_restaurer, NULL);

            g_tray_entry_pause = SDL_InsertTrayEntryAt(menu, -1, "Pause", SDL_TRAYENTRY_BUTTON);
            if (g_tray_entry_pause) SDL_SetTrayEntryCallback(g_tray_entry_pause, on_tray_pause, NULL);

            SDL_TrayEntry *e_add5 = SDL_InsertTrayEntryAt(menu, -1, "+5 minutes", SDL_TRAYENTRY_BUTTON);
            if (e_add5) SDL_SetTrayEntryCallback(e_add5, on_tray_ajouter_5m, NULL);

            SDL_TrayEntry *e_sub5 = SDL_InsertTrayEntryAt(menu, -1, "-5 minutes", SDL_TRAYENTRY_BUTTON);
            if (e_sub5) SDL_SetTrayEntryCallback(e_sub5, on_tray_retirer_5m, NULL);

            SDL_TrayEntry *e_quit = SDL_InsertTrayEntryAt(menu, -1, "Quitter", SDL_TRAYENTRY_BUTTON);
            if (e_quit) SDL_SetTrayEntryCallback(e_quit, on_tray_quitter, NULL);
        }
    }
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
            actualiser_tray_state();
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

    // Alerte automatique à moins de 5 minutes restantes :
    // On restaure immédiatement la fenêtre et on la place au premier plan
    if (g_minutes <= 5 && g_heures <= 0 && g_only_one)
    {
        restaurer_fenetre();
        if (g_window)
        {
            SDL_SetWindowAlwaysOnTop(g_window, true);
        }
        g_only_one = false;
    }

    actualiser_affichage();
    actualiser_tray_state();
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
    }
    else
    {
        // Reprise du décompte
        g_playing = 1;
        g_timer = SDL_AddTimer(1000, timer_callback, NULL);
    }
    actualiser_affichage();
    actualiser_tray_state();
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
    g_window = SDL_CreateWindow(TITRE_APPLICATION, FENETRE_L, FENETRE_H, 0);
    if (!g_window)
    {
        fprintf(stderr, "Impossible de creer la fenetre SDL 3: %s\n", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return EXIT_FAILURE;
    }

    g_renderer = SDL_CreateRenderer(g_window, NULL);
    if (!g_renderer)
    {
        fprintf(stderr, "Impossible de creer le renderer SDL 3: %s\n", SDL_GetError());
        SDL_DestroyWindow(g_window);
        TTF_Quit();
        SDL_Quit();
        return EXIT_FAILURE;
    }

    // Initialisation du System Tray natif (Windows & Linux) avec icônes normale et pause
    init_systray();

    // Application de l'icône initiale à la fenêtre
    if (g_icon_normal)
    {
        SDL_SetWindowIcon(g_window, g_icon_normal);
    }

    // Chargement de la police libre (Patrick Hand sous licence SIL OFL)
    std::string font_path = get_asset_path("PatrickHand-Regular.ttf");
    g_police = TTF_OpenFont(font_path.c_str(), 32);
    if (!g_police)
    {
        fprintf(stderr, "Erreur chargement police '%s': %s\n", font_path.c_str(), SDL_GetError());
        if (g_tray) SDL_DestroyTray(g_tray);
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
    actualiser_tray_state();
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

            case SDL_EVENT_MOUSE_MOTION:
            {
                float mx = event.motion.x;
                float my = event.motion.y;
                bool hm = (mx >= BTN_MINUS_X && mx < BTN_MINUS_X + BTN_TAILLE && my >= BTN_Y && my < BTN_Y + BTN_TAILLE);
                bool hp = (mx >= BTN_PLUS_X && mx < BTN_PLUS_X + BTN_TAILLE && my >= BTN_Y && my < BTN_Y + BTN_TAILLE);
                if (hm != g_hover_minus || hp != g_hover_plus)
                {
                    g_hover_minus = hm;
                    g_hover_plus = hp;
                    actualiser_affichage();
                }
                break;
            }

            case SDL_EVENT_MOUSE_WHEEL:
            {
                SDL_Keymod mod = SDL_GetModState();
                if (event.wheel.y > 0)
                {
                    ajuster_temps((mod & SDL_KMOD_SHIFT) ? 300 : 60);
                }
                else if (event.wheel.y < 0)
                {
                    ajuster_temps((mod & SDL_KMOD_SHIFT) ? -300 : -60);
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
                    // Clic sur bouton - : -5min par défaut, -1min si clic droit ou Shift, -15min si Ctrl
                    int delta = (btn == SDL_BUTTON_RIGHT || (mod & SDL_KMOD_SHIFT)) ? -60 : ((mod & SDL_KMOD_CTRL) ? -900 : -300);
                    ajuster_temps(delta);
                }
                else if (bx >= BTN_PLUS_X && bx < BTN_PLUS_X + BTN_TAILLE && by >= BTN_Y && by < BTN_Y + BTN_TAILLE)
                {
                    // Clic sur bouton + : +5min par défaut, +1min si clic droit ou Shift, +15min si Ctrl
                    int delta = (btn == SDL_BUTTON_RIGHT || (mod & SDL_KMOD_SHIFT)) ? 60 : ((mod & SDL_KMOD_CTRL) ? 900 : 300);
                    ajuster_temps(delta);
                }
                else if (btn == SDL_BUTTON_LEFT)
                {
                    // Clic en zone centrale : basculer pause/lecture
                    basculer_pause();
                }
                break;
            }

            case SDL_EVENT_WINDOW_MINIMIZED:
                // Ignorer les événements de minimisation résiduels générés
                // par le système pendant la transition de restauration (délai de 500ms)
                if (SDL_GetTicks() - g_last_restore_time > 500)
                {
                    g_is_minimized = true;
                    SDL_HideWindow(g_window);
                }
                break;

            case SDL_EVENT_WINDOW_RESTORED:
            case SDL_EVENT_WINDOW_SHOWN:
                g_is_minimized = false;
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
    if (g_tray)
    {
        SDL_DestroyTray(g_tray);
        g_tray = NULL;
    }
    if (g_icon_normal)
    {
        SDL_DestroySurface(g_icon_normal);
        g_icon_normal = NULL;
    }
    if (g_icon_pause)
    {
        SDL_DestroySurface(g_icon_pause);
        g_icon_pause = NULL;
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
