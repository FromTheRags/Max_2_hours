#include "ui.h"
#include "common.h"
#include "timer.h"
#include <cstdio>

#if defined(__APPLE__)
    #include <SDL_ttf/SDL_ttf.h>
#else
    #include <SDL_ttf.h>
#endif

static SDL_Surface *g_ecran = NULL;
static TTF_Font *g_police = NULL;
static SDL_Surface *g_play_surface = NULL;
static SDL_Surface *g_icon_normal = NULL;
static SDL_Surface *g_icon_pause = NULL;
static bool g_hover_minus = false;
static bool g_hover_plus = false;

SDL_Surface *ui_get_screen() { return g_ecran; }

void ui_set_icon_normal()
{
    if (g_icon_normal)
    {
        SDL_WM_SetIcon(g_icon_normal, NULL);
    }
}

void ui_set_icon_pause()
{
    if (g_icon_pause)
    {
        SDL_WM_SetIcon(g_icon_pause, NULL);
    }
}

static void dessiner_bouton(SDL_Surface *surface, int x, int y, int taille, bool est_plus, bool hover)
{
    SDL_Rect r_bg = { (Sint16)x, (Sint16)y, (Uint16)taille, (Uint16)taille };
    Uint32 col_bg = hover ? SDL_MapRGB(surface->format, 255, 123, 0)
                          : SDL_MapRGB(surface->format, 246, 246, 248);
    Uint32 col_bordure = hover ? SDL_MapRGB(surface->format, 220, 95, 0)
                               : SDL_MapRGB(surface->format, 215, 215, 220);
    Uint32 col_symbole = hover ? SDL_MapRGB(surface->format, 255, 255, 255)
                               : SDL_MapRGB(surface->format, 255, 123, 0);

    // Fond
    SDL_FillRect(surface, &r_bg, col_bg);

    // Bordure 1px
    SDL_Rect bord_h = { (Sint16)x, (Sint16)y, (Uint16)taille, 1 };
    SDL_Rect bord_b = { (Sint16)x, (Sint16)(y + taille - 1), (Uint16)taille, 1 };
    SDL_Rect bord_g = { (Sint16)x, (Sint16)y, 1, (Uint16)taille };
    SDL_Rect bord_d = { (Sint16)(x + taille - 1), (Sint16)y, 1, (Uint16)taille };
    SDL_FillRect(surface, &bord_h, col_bordure);
    SDL_FillRect(surface, &bord_b, col_bordure);
    SDL_FillRect(surface, &bord_g, col_bordure);
    SDL_FillRect(surface, &bord_d, col_bordure);

    // Barre horizontale (- ou +)
    int barre_w = 14;
    int barre_h = 3;
    SDL_Rect r_h = {
        (Sint16)(x + (taille - barre_w) / 2),
        (Sint16)(y + (taille - barre_h) / 2),
        (Uint16)barre_w,
        (Uint16)barre_h
    };
    SDL_FillRect(surface, &r_h, col_symbole);

    // Barre verticale pour +
    if (est_plus)
    {
        SDL_Rect r_v = {
            (Sint16)(x + (taille - barre_h) / 2),
            (Sint16)(y + (taille - barre_w) / 2),
            (Uint16)barre_h,
            (Uint16)barre_w
        };
        SDL_FillRect(surface, &r_v, col_symbole);
    }
}

bool ui_init()
{
    if (TTF_Init() < 0)
    {
        fprintf(stderr, "Erreur initialisation SDL_ttf: %s\n", TTF_GetError());
        return false;
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

    // Titre de la fenêtre
    SDL_WM_SetCaption(TITRE_APPLICATION, NULL);

    // Création de la surface d'écran
    g_ecran = SDL_SetVideoMode(FENETRE_L, FENETRE_H, 16, SDL_HWSURFACE | SDL_DOUBLEBUF);
    if (!g_ecran)
    {
        fprintf(stderr, "Impossible d'ouvrir la fenêtre SDL: %s\n", SDL_GetError());
        return false;
    }

    // Chargement de la police Patrick Hand
    std::string font_path = get_asset_path("PatrickHand-Regular.ttf");
    g_police = TTF_OpenFont(font_path.c_str(), 32);
    if (!g_police)
    {
        fprintf(stderr, "Erreur chargement police '%s': %s\n", font_path.c_str(), TTF_GetError());
        return false;
    }

    // Chargement de l'icône Play
    std::string play_path = get_asset_path("play.bmp");
    g_play_surface = SDL_LoadBMP(play_path.c_str());
    if (g_play_surface)
    {
        SDL_SetColorKey(g_play_surface, SDL_SRCCOLORKEY, SDL_MapRGB(g_play_surface->format, 255, 255, 255));
    }

    return true;
}

void ui_cleanup()
{
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
}

void ui_set_hover(bool hover_minus, bool hover_plus)
{
    if (hover_minus != g_hover_minus || hover_plus != g_hover_plus)
    {
        g_hover_minus = hover_minus;
        g_hover_plus = hover_plus;
        ui_actualiser();
    }
}

void ui_actualiser()
{
    if (!g_ecran) return;

    // Fond blanc
    SDL_FillRect(g_ecran, NULL, SDL_MapRGB(g_ecran->format, 255, 255, 255));

    // Dessin des boutons - et +
    dessiner_bouton(g_ecran, BTN_MINUS_X, BTN_Y, BTN_TAILLE, false, g_hover_minus);
    dessiner_bouton(g_ecran, BTN_PLUS_X, BTN_Y, BTN_TAILLE, true, g_hover_plus);

    if (g_playing)
    {
        if (g_police)
        {
            char temps[64] = {0};
            snprintf(temps, sizeof(temps), "Temps: %02d : %02d : %02d", g_heures, g_minutes, g_secondes);

            SDL_Surface *texte = TTF_RenderText_Blended(g_police, temps, COULEUR_ORANGE);
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
        // En pause : afficher l'icône de reprise et le temps actuel
        if (g_police)
        {
            char temps[64] = {0};
            snprintf(temps, sizeof(temps), "%02d : %02d : %02d", g_heures, g_minutes, g_secondes);
            SDL_Surface *texte = TTF_RenderText_Blended(g_police, temps, COULEUR_ORANGE);

            int play_w = g_play_surface ? g_play_surface->w : 0;
            int play_h = g_play_surface ? g_play_surface->h : 0;
            int txt_w = texte ? texte->w : 0;
            int txt_h = texte ? texte->h : 0;
            int espacement = 12;
            int total_w = play_w + (play_w && txt_w ? espacement : 0) + txt_w;
            int start_x = (g_ecran->w - total_w) / 2;

            if (g_play_surface)
            {
                SDL_Rect pos_play;
                pos_play.x = start_x;
                pos_play.y = (g_ecran->h - play_h) / 2;
                SDL_BlitSurface(g_play_surface, NULL, g_ecran, &pos_play);
            }

            if (texte)
            {
                SDL_Rect pos_txt;
                pos_txt.x = start_x + (play_w ? play_w + espacement : 0);
                pos_txt.y = (g_ecran->h - txt_h) / 2;
                SDL_BlitSurface(texte, NULL, g_ecran, &pos_txt);
                SDL_FreeSurface(texte);
            }
        }
    }

    SDL_Flip(g_ecran);
}
