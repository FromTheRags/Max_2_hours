# Max_2_hours (Protect & Perf) - Minuteur SDL 1.2

[![License: GPL-3.0-or-later](https://img.shields.io/badge/License-GPLv3-blue.svg)](LICENSE)
[![Library: SDL 1.2](https://img.shields.io/badge/Library-SDL%201.2-green.svg)](https://www.libsdl.org/)
[![CMake](https://img.shields.io/badge/Build-CMake-064F8C.svg?logo=cmake&logoColor=white)](https://cmake.org/)

Un minuteur de session (2 heures) en C++ avec SDL 1.2 et SDL_ttf, initialement conçu pour limiter le temps d'écran et prévenir la fatigue visuelle.
Ce projet d'archive a été nettoyé et mis en conformité **libre de droits** (GPLv3, SIL OFL 1.1 et CC0 1.0) avec une chaîne de compilation moderne CMake.

---

## Fonctionnalités

- **Décompte de 2 heures** : affichage grand format du temps restant (`Temps: 02 : 00 : 00`) en orange sur fond blanc.
- **Pause / Reprise au clic** : un simple clic sur la fenêtre met le décompte en pause et affiche l'icône de lecture ; un second clic relance le chronomètre.
- **Rappel de sécurité à 5 minutes** : lorsque moins de 5 minutes restent au chronomètre, la fenêtre se restaure et se place au premier plan (`HWND_TOPMOST` sous Windows).
- **Ressources 100 % libres de droits** :
  - Police manuscrite open-source **Patrick Hand** (SIL OFL 1.1).
  - Graphismes originaux dédiés au domaine public (CC0 1.0).

---

## Compilation avec CMake

### Prérequis
- CMake 3.16 ou supérieur
- Un compilateur C/C++ supportant C++17 (MSVC, MinGW GCC, Clang)
- SDL 1.2 et SDL_ttf (fournis dans `cmake/sdl12` pour Windows)

### Instructions de compilation

```bash
# Génération des fichiers de build
cmake -B build

# Compilation de l'exécutable
cmake --build build
```

L'exécutable `max_2_hours.exe` est généré dans le répertoire `build/`, accompagné de l'ensemble de ses dépendances d'exécution (DLLs SDL 1.2 et répertoire `assets/`).

---

## Licences

- Code source : **GNU GPL v3.0** ([LICENSE](LICENSE))
- Typographie : **SIL Open Font License 1.1** ([OFL.txt](OFL.txt))
- Assets graphiques : **Creative Commons Zero 1.0** ([LICENSES.md](LICENSES.md))
