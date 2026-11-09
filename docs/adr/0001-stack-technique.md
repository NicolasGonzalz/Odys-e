# ADR 0001 — Stack technique

- **Statut :** accepté
- **Date :** 2026-11-09

## Contexte

Projet solo, ambition long terme, objectif d'apprentissage et de démonstration sur deux axes :
un serveur autoritaire robuste et un réseau temps réel. Expérience préalable en C++ (Zappy, Raytracer, Arcade).

## Décision

- **C++20** pour le client, le serveur et le code partagé.
- **CMake** avec `FetchContent` ; chaque dépendance est épinglée sur un tag précis.
- **raylib** pour le rendu client : fenêtre, entrées, 3D, audio, sans imposer d'architecture.
- **Réseau UDP écrit à la main** plutôt qu'une bibliothèque (ENet, GameNetworkingSockets) : c'est le cœur
  technique du projet. ENet reste le plan de repli si la couche maison devient un frein.
- **doctest** pour les tests unitaires (léger, rapide à compiler).
- **SQLite** prévu pour la persistance (M5).
- Style **3D low-poly**.

## Alternatives écartées

- **Godot / Unity / Unreal** : productifs, mais masquent précisément ce qu'on veut maîtriser (boucle, réseau, synchro).
- **Rust + Bevy** : intéressant, mais coût d'apprentissage qui retarderait le cœur du projet.
- **Voxel pur** : le streaming et la synchro des chunks modifiables alourdissent le réseau dès le départ ;
  le low-poly sur heightmap laisse la porte ouverte à une évolution ultérieure.

## Conséquences

- Plus de travail sur l'outillage (UI, chargement d'assets, éditeur) qu'avec un moteur.
- Contrôle total sur la boucle de jeu, la mémoire et le format réseau.
- Le serveur reste sans dépendance graphique et peut être compilé seul (`-DODYSSEE_BUILD_CLIENT=OFF`).
