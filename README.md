# Max_2_hours (Protect & Perf) - Minuteur SDL 3 Remastérisé

[![Build & Package (SDL 3)](https://github.com/FromTheRags/Max_2_hours/actions/workflows/build.yml/badge.svg?branch=SDL_3)](https://github.com/FromTheRags/Max_2_hours/actions/workflows/build.yml)
[![License: GPL-3.0-or-later](https://img.shields.io/badge/License-GPLv3-blue.svg)](LICENSE)
[![Library: SDL 3](https://img.shields.io/badge/Library-SDL%203-blue.svg)](https://www.libsdl.org/)
[![CMake](https://img.shields.io/badge/Build-CMake-064F8C.svg?logo=cmake&logoColor=white)](https://cmake.org/)

> [!NOTE]
> 📜 **Vous recherchez la version d'origine rétro SDL 1.2 ?** Rendez-vous sur la branche [`main`](https://github.com/FromTheRags/Max_2_hours/tree/main) !
> Cette branche `SDL_3` constitue la **version moderne remastérisée** de l'application, tirant parti de **SDL 3** et **SDL3_ttf** avec rendu GPU accéléré, gestion multiplateforme native des fenêtres et automatisation CI/CD.

Un minuteur de session (2 heures) en C++ avec SDL 3 et SDL3_ttf, initialement conçu pour limiter le temps d'écran et prévenir la fatigue visuelle.
**Le code a été entièrement remastérisé** : architecture moderne SDL 3, élimination des dépendances obsolètes, assainissement légal 100 % libre de droits (GPLv3, SIL OFL 1.1 et CC0 1.0) et pipeline CI/CD automatisée sous Linux et Windows.

---

## ✨ Fonctionnalités remastérisées (SDL 3)

- **Moteur moderne SDL 3** : Rendu graphique 2D accéléré par GPU (`SDL_Renderer` / `SDL_Texture`) avec synchronisation verticale.
- **Décompte de 2 heures** : Affichage net et fluide du temps restant (`Temps: 02 : 00 : 00`) en orange sur fond blanc avec centrage automatique.
- **Pause / Reprise au clic** : Mise en pause du minuteur en cliquant sur la fenêtre, avec affichage d'une icône de reprise (Play) vectorisée.
- **Gestion des fenêtres native SDL 3** :
  - Restauration automatique et mise au premier plan native via `SDL_SetWindowAlwaysOnTop` lors des 5 dernières minutes sans recourir à des hacks Win32 bas niveau.
- **Mise en veille prolongée automatique** : Déclenchement de l'hibernation du système (`shutdown /h` sous Windows, `systemctl hibernate` sous Linux) dès que le décompte de 2 heures est achevé.
- **Assets 100 % libres de droits** :
  - Typographie manuscrite open-source **[Patrick Hand](https://fonts.google.com/specimen/Patrick+Hand)** (SIL OFL 1.1).
  - Graphismes originaux dédiés au domaine public (**CC0 1.0**).

---

## 🛠️ Compilation avec CMake

### Prérequis
- CMake 3.20 ou supérieur
- Un compilateur C/C++ supportant C++17 (MSVC, MinGW GCC, Clang)
- SDL 3 et SDL3_ttf (détectés automatiquement via le système, les bibliothèques incluses sous Windows dans `cmake/sdl3`, ou via `FetchContent`)

### Instructions de compilation

```bash
# Génération des fichiers de build
cmake -B build

# Compilation de l'exécutable
cmake --build build
```

L'exécutable `max_2_hours` est généré dans le répertoire `build/`, prêt à l'emploi avec ses dépendances et le dossier `assets/`.

---

## 🤖 CI/CD Automatisée

Le workflow GitHub Actions (`.github/workflows/build.yml`) valide chaque commit sur :
- **Ubuntu Linux** : via l'action officielle `libsdl-org/setup-sdl`, compilation Ninja et publication d'une archive `.tar.gz`.
- **Windows** : via MSYS2 MinGW-w64 (`sdl3`, `sdl3-ttf`), compilation Ninja et empaquetage autonome `.zip` incluant les DLLs requises.

---

## 📄 Licences

- Code source : **GNU GPL v3.0** ([LICENSE](LICENSE))
- Typographie : **SIL Open Font License 1.1** ([OFL.txt](OFL.txt))
- Assets graphiques : **Creative Commons Zero 1.0** ([LICENSES.md](LICENSES.md))
