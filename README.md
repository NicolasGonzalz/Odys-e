# Odyssée

> Nom de code : **Odyssée** · projet personnel, en développement

Jeu en ligne multijoueur d'aventure et de survie en monde persistant, construit entièrement seul, de A à Z.
Les joueurs partagent un monde où ils explorent, récoltent, fabriquent et progressent, en solo comme à plusieurs.

Le défi est surtout invisible :

- un **serveur de jeu autoritaire** qui garde le monde cohérent pour tous les joueurs (coop jusqu'à 4) ;
- un **réseau temps réel** assez fluide pour que tout reste réactif malgré la latence (prédiction côté client, réconciliation, interpolation).

C'est la suite logique de mes projets réseau et 3D (Zappy, Raytracer, Arcade).

## Stack

| Domaine | Choix |
|---|---|
| Langage | C++20 |
| Build | CMake ≥ 3.24, dépendances via `FetchContent` |
| Client | [Godot 4](https://godotengine.org/) + GDExtension C++ — *proposé, voir [ADR 0002](docs/adr/0002-client-godot.md)* (prototype actuel : raylib) |
| Serveur | C++20 headless, autoritaire — homelab Proxmox ou local (solo) |
| Assets | Figma (2D, UI, concepts) → Blender (3D, animations) → glTF |
| Plateformes | Linux, Windows |
| Réseau | UDP maison, fiabilité sélective (voir [docs/NETWORK.md](docs/NETWORK.md)) |
| Tests | [doctest](https://github.com/doctest/doctest) |

## Structure

```
.
├── shared/   # Code commun client/serveur : protocole, sérialisation, simulation
├── server/   # Serveur dédié autoritaire (headless)
├── client/   # Client 3D (prototype raylib, Godot à venir)
├── tests/    # Tests unitaires
├── cmake/    # Modules CMake (warnings, dépendances)
├── docs/     # Conception : vision, architecture, réseau, roadmap, planning, ADR
├── planning/ # Source du planning (tâches + jalons, TSV)
└── scripts/  # Outils (planning.py : génère PLANNING.md et les issues GitHub)
```

## Compiler

Prérequis (Ubuntu/Debian) :

```sh
sudo apt install build-essential cmake git \
    libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libgl1-mesa-dev
```

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
ctest --test-dir build --output-on-failure
```

Options CMake : `ODYSSEE_BUILD_CLIENT`, `ODYSSEE_BUILD_SERVER`, `ODYSSEE_BUILD_TESTS`, `ODYSSEE_SANITIZERS`.
Pour compiler uniquement le serveur (sans dépendances graphiques) : `-DODYSSEE_BUILD_CLIENT=OFF`.

## Lancer

```sh
./build/server/odyssee_server [port]   # port UDP par défaut : 27015
./build/client/odyssee_client
```

## Documentation

- [Vision & game design](docs/VISION.md)
- [Architecture technique](docs/ARCHITECTURE.md)
- [Modèle réseau & protocole](docs/NETWORK.md)
- [Modes de jeu & hébergement](docs/MODES.md)
- [Roadmap & jalons](docs/ROADMAP.md)
- [Planning semaine par semaine](docs/PLANNING.md)
- [Décisions d'architecture (ADR)](docs/adr/)

## Statut

🚧 **M0 — Fondations.** Voir la [roadmap](docs/ROADMAP.md).
