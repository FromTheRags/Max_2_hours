#include "ui.h"
#include "common.h"
#include "timer.h"
#include <SDL3_ttf/SDL_ttf.h>

static SDL_Window *g_window = NULL;
static SDL_Renderer *g_renderer = NULL;
static TTF_Font *g_police = NULL;
static SDL_Texture *g_play_texture = NULL;
static int g_play_w = 0;
static int g_play_h = 0;
static bool g_hover_minus = false;
static bool g_hover_plus = false;

SDL_Window *ui_get_window() { return g_window; }
SDL_Renderer *ui_get_renderer() { return g_renderer; }

static void dessiner_bouton(SDL_Renderer *renderer, float x, float y, float taille, bool est_plus, bool hover)
{
    SDL_FRect r_bg = { x, y, taille, taille };

    // Fond
    if (hover)
        SDL_SetRenderDrawColor(renderer, 255, 123, 0, 255);
    else
        SDL_SetRenderDrawColor(renderer, 246, 246, 248, 255);
    SDL_RenderFillRect(renderer, &r_bg);

    // Bordure
    if (hover)
        SDL_SetRenderDrawColor(renderer, 220, 95, 0, 255);
    else
        SDL_SetRenderDrawColor(renderer, 215, 215, 220, 255);
    SDL_RenderRect(renderer, &r_bg);

    // Symbole
    if (hover)
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    else
        SDL_SetRenderDrawColor(renderer, 255, 123, 0, 255);

    // Barre horizontale
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

bool ui_init()
{
    if (!TTF_Init())
    {
        SDL_Log("Erreur initialisation SDL_ttf: %s", SDL_GetError());
        return false;
    }

    g_window = SDL_CreateWindow(
        TITRE_APPLICATION,
        FENETRE_L,
        FENETRE_H,
        SDL_WINDOW_ALWAYS_ON_TOP
    );
    if (!g_window)
    {
        SDL_Log("Impossible de créer la fenêtre: %s", SDL_GetError());
        return false;
    }

    g_renderer = SDL_CreateRenderer(g_window, NULL);
    if (!g_renderer)
    {
        SDL_Log("Impossible de créer le renderer: %s", SDL_GetError());
        return false;
    }

    // Chargement de la police
    std::string font_path = get_asset_path("PatrickHand-Regular.ttf");
    g_police = TTF_OpenFont(font_path.c_str(), 32);
    if (!g_police)
    {
        SDL_Log("Erreur chargement police '%s': %s", font_path.c_str(), SDL_GetError());
        return false;
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

    return true;
}

void ui_cleanup()
{
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

            SDL_Surface *texte_surf = TTF_RenderText_Blended(g_police, temps, 0, COULEUR_ORANGE);
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
            SDL_Surface *texte_surf = TTF_RenderText_Blended(g_police, temps, 0, COULEUR_ORANGE);

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
