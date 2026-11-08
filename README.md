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

- **Titre de fenêtre lisible** : Dimensions optimisées (360x60) pour garantir la visibilité complète du titre (« Protect & Perf - 2 Heures ») dans la barre de titre du système sans être tronqué par les boutons de contrôle (Réduire, Agrandir, Fermer).
- **Intégration System Tray (Barre des tâches / Notification)** :
  - **Minimisation dans le Systray** : Réduire la fenêtre la masque automatiquement de l'écran et de la barre des tâches pour la loger dans la zone de notification (Windows et Linux).
  - **Infobulle en temps réel** : Survoler l'icône dans la zone de notification affiche le temps restant en direct.
  - **Menu contextuel** : Clic droit permettant d'afficher la fenêtre, mettre en pause / reprendre le minuteur, ou quitter.
  - **Rappel automatique à 5 minutes** : La fenêtre se restaure automatiquement depuis le systray et se place au premier plan (`SDL_SetWindowAlwaysOnTop`).
- **Moteur moderne SDL 3** : Rendu graphique 2D accéléré par GPU (`SDL_Renderer` / `SDL_Texture`) avec synchronisation verticale.
- **Décompte de 2 heures** : Affichage net et fluide du temps restant (`Temps: 02 : 00 : 00`) en orange sur fond blanc avec centrage automatique.
- **Pause / Reprise au clic** : Mise en pause du minuteur en cliquant sur la fenêtre, avec affichage d'une icône de reprise (Play) vectorisée.
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
