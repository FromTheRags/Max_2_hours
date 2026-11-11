#ifndef COMMON_H
#define COMMON_H

#include <string>

#if defined(__APPLE__)
    #include <SDL/SDL.h>
#else
    #include <SDL.h>
#endif

// Dimensions de la fenêtre et titre
inline constexpr const char *TITRE_APPLICATION = "Protect & Perf";
inline constexpr int FENETRE_L = 380;
inline constexpr int FENETRE_H = 60;

// Dimensions et positions des boutons - et +
inline constexpr int BTN_TAILLE = 32;
inline constexpr int BTN_Y = (FENETRE_H - BTN_TAILLE) / 2;
inline constexpr int BTN_MINUS_X = 12;
inline constexpr int BTN_PLUS_X = FENETRE_L - 12 - BTN_TAILLE;

// Couleur orange de référence
inline constexpr SDL_Color COULEUR_ORANGE = {255, 123, 0, 0};

// Version de l'application (définie par CMake via -DAPP_VERSION)
#ifndef APP_VERSION
    #define APP_VERSION "dev"
#endif

// Chaîne de version affichée dans l'info-bulle : "v1.0.0 (SDL 1.2.15)"
// (version de SDL réellement liée à l'exécution)
std::string get_version_string();

// Résolution portable des chemins d'accès aux assets
std::string get_asset_path(const char *name);

#endif // COMMON_H
