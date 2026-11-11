# Odyssée

[![CI](https://github.com/NicolasGonzalz/Odys-e/actions/workflows/ci.yml/badge.svg)](https://github.com/NicolasGonzalz/Odys-e/actions/workflows/ci.yml)

> Nom de code : **Odyssée** · projet personnel, en développement

Jeu en ligne multijoueur d'aventure et de survie en monde persistant, construit à deux, de A à Z : le code et le serveur d'un côté, le moteur et l'art de l'autre.
Les joueurs partagent un monde où ils explorent, récoltent, fabriquent et progressent, en solo comme à plusieurs.

Le défi est surtout invisible :

- un **serveur de jeu autoritaire** qui garde le monde cohérent pour tous les joueurs (coop jusqu'à 4) ;
- un **réseau temps réel** assez fluide pour que tout reste réactif malgré la latence (prédiction côté client, réconciliation, interpolation).

C'est la suite logique de mes projets réseau et 3D (Zappy, Raytracer, Arcade).

![Odyssée — les cinq zones](art/concepts/ambiances/zones-1-a-5-planche-b.jpeg)

*Concepts de la direction artistique (voir [art/README.md](art/README.md)) : les cinq zones, de la plaine du réveil à la forêt du Cœur.*

## Stack

| Domaine | Choix |
|---|---|
| Langage | C++20 |
| Build | CMake ≥ 3.24, dépendances via `FetchContent` |
| Client | [Unreal Engine **5.6**](https://www.unrealengine.com/) (version figée) + plugin C++ `OdysseeNet` — voir [ADR 0003](docs/adr/0003-client-unreal.md) |
| Client de debug | raylib (Linux, tests serveur et réseau sans Unreal) |
| Serveur | C++20 headless, autoritaire — homelab Proxmox ou local (solo) |
| Assets | Figma (2D, UI, concepts) → Blender (3D, animations) → Unreal (Git LFS) |
| Plateformes | Linux, Windows |
| Réseau | UDP maison, fiabilité sélective (voir [docs/NETWORK.md](docs/NETWORK.md)) |
| Tests | [doctest](https://github.com/doctest/doctest) |

## Structure

```
.
├── shared/   # Code commun client/serveur : protocole, sérialisation, simulation
├── server/   # Serveur dédié autoritaire (headless)
├── client/   # Client de debug raylib (tests serveur/réseau sous Linux)
├── UnrealEngine/Odysse/  # Client Unreal 5.6 (C++ dans Source/, assets via Git LFS)
├── tests/    # Tests unitaires
├── cmake/    # Modules CMake (warnings, dépendances)
├── art/      # Références visuelles (concepts de la direction artistique, via Git LFS)
├── docs/     # Conception : vision, architecture, réseau, roadmap, planning, ADR
├── planning/ # Source du planning (tâches + jalons, TSV)
└── scripts/  # Outils (planning.py : génère PLANNING.md et les issues GitHub)
```

## Git LFS

Les fichiers binaires (Unreal, art, audio) passent par Git LFS : `sudo apt install git-lfs && git lfs install` avant de cloner ou de committer des assets.

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

Sous **Windows** (Visual Studio 2022, utile pour le mode solo et le plugin Unreal) :

```powershell
cmake -S . -B build -DODYSSEE_BUILD_CLIENT=OFF
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```

Options CMake : `ODYSSEE_BUILD_CLIENT`, `ODYSSEE_BUILD_SERVER`, `ODYSSEE_BUILD_TESTS`, `ODYSSEE_SANITIZERS`.
Pour compiler uniquement le serveur (sans dépendances graphiques) : `-DODYSSEE_BUILD_CLIENT=OFF`.

## Lancer

```sh
./build/server/odyssee_server [port]   # port UDP par défaut : 27015
./build/server/odyssee_server --lag 120 --jitter 30 --loss 5   # simule un mauvais réseau (ms, %)
./build/client/odyssee_client
```

## Documentation

- [Vision & game design](docs/VISION.md)
- [Architecture technique](docs/ARCHITECTURE.md)
- [Modèle réseau & protocole](docs/NETWORK.md)
- [Modes de jeu & hébergement](docs/MODES.md)
- [Équipe & façon de travailler](docs/TEAM.md)
- [Lore](docs/LORE.md) · [Survivants](docs/CHARACTERS.md) · [Zones & bestiaire](docs/ZONES.md)
- [Roadmap & jalons](docs/ROADMAP.md)
- [Planning semaine par semaine](docs/PLANNING.md)
- [Décisions d'architecture (ADR)](docs/adr/)

## Statut

🚧 **M0 — Fondations.** Voir la [roadmap](docs/ROADMAP.md).
