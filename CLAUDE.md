# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Projet

**Odyssée** (nom de code) : jeu de survie/aventure en coop (1 à 4 joueurs), monde persistant, vue 1re personne,
univers post-apo « retour à l'état sauvage ». Fait à deux : Nicolas (code C++, serveur, réseau, infra) et un coéquipier
(Unreal, Figma, Blender, cartes). **Les rôles sont interchangeables** : chacun peut reprendre une tâche de l'autre,
donc ne pas supposer qui travaille à partir du domaine de la tâche.

Docs et commentaires de conception en **français** ; code et identifiants en **anglais**.

## Commandes

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug     # configure (télécharge raylib + doctest via FetchContent)
cmake --build build -j                           # compile tout
ctest --test-dir build --output-on-failure       # lance les tests
./build/tests/odyssee_tests -tc="ByteBuffer*"    # un seul test / groupe (filtre doctest sur le nom du TEST_CASE)
./build/server/odyssee_server [port]             # serveur (UDP 27015 par défaut), Ctrl+C pour l'arrêter proprement
./build/client/odyssee_client                    # client de debug raylib
clang-format -i <fichiers>                       # formatage (.clang-format à la racine)
```

Options CMake : `ODYSSEE_BUILD_CLIENT` (OFF = pas de dépendance graphique), `ODYSSEE_BUILD_SERVER`,
`ODYSSEE_BUILD_TESTS`, `ODYSSEE_SANITIZERS` (ASan + UBSan, activé dans la CI).
Warnings stricts (`cmake/CompilerWarnings.cmake`, dont `-Wconversion -Wsign-conversion -Wold-style-cast`) : le code doit compiler sans warning.

Planning (source unique = `planning/tasks.tsv` + `planning/milestones.tsv`, jamais éditer `docs/PLANNING.md` ni les issues à la main) :

```sh
scripts/planning.py render         # régénère docs/PLANNING.md
scripts/planning.py github         # crée/synchronise labels, jalons et issues GitHub (idempotent)
scripts/planning.py project 1      # range les issues dans le GitHub Project n°1 et remplit ses champs
```

## Architecture

Trois cibles CMake autour d'une bibliothèque commune :

- **`shared/`** (`odyssee::shared`) : code commun client/serveur — config (`core/Config.hpp` : tick rate 20 Hz,
  port, taille max de paquet 1200 o, version de protocole), sérialisation binaire little-endian
  (`net/ByteBuffer.hpp` : `ByteReader` renvoie `std::optional`, toute donnée réseau est considérée hostile),
  et à venir : couche UDP, simulation déterministe (`sim/`), chargement de carte (`world/`).
- **`server/`** : serveur **autoritaire** headless, boucle à pas fixe (`Server::run` rattrape les ticks en retard).
  Il possède l'unique état de vérité du monde.
- **`client/`** : client de **debug** raylib (tests serveur/réseau sous Linux, base des bots). Le vrai client est
  **Unreal Engine 5** (à venir dans `client-unreal/`), branché via un plugin C++ `OdysseeNet` qui compile les sources de `shared/`.

Principes structurants (détails dans `docs/ARCHITECTURE.md`, `docs/NETWORK.md`, `docs/adr/0003-client-unreal.md`) :

- **Pas de réplication Unreal.** Le client n'envoie que des inputs ; il prédit son joueur avec la **même** simulation
  `shared/sim` que le serveur, puis réconcilie. Les autres entités sont interpolées.
- **Les collisions de référence viennent de la carte exportée** (fichier monde produit depuis l'éditeur Unreal),
  jamais de la physique Unreal, sinon client et serveur divergent.
- **`shared/` doit compiler sous GCC, Clang et MSVC, sans exceptions ni RTTI** (désactivés par Unreal) ;
  unités en mètres, Z vers le haut (conversion vers les centimètres Unreal dans le plugin).
- Le solo lance le même `odyssee_server` en local : il y a toujours un serveur, un seul chemin de code.
- Réseau : UDP maison, trois canaux (non fiable / fiable ordonné / fiable non ordonné), seq + ack + bitfield,
  zone d'intérêt par distance (pas par direction du regard).

## Conventions

- Membres privés préfixés `_`, types `PascalCase`, fonctions `camelCase`, constantes `kPascalCase`,
  namespaces `odyssee::net`, `odyssee::sim`, `odyssee::server`…
- **Une tâche = une branche = une PR**, branche nommée d'après l'ID du planning (`t004-logger`) ; référencer
  l'issue (`Closes #4`). Ne jamais fusionner sur `main` avec la CI rouge.
- **Git LFS** pour tous les binaires (`.uasset`, `.umap`, `.blend`, images, audio — voir `.gitattributes`) ;
  `git lfs lock` avant de modifier une carte ou un asset Unreal.
- Décisions structurantes consignées en ADR (`docs/adr/`), une nouvelle ADR remplace l'ancienne plutôt que de la réécrire.
- Les noms du lore (Gaïa, Odyssée, zones, survivants) sont **provisoires**.

## Docs de conception

`docs/VISION.md` (game design, règles coop) · `docs/LORE.md` (mystère, fins) · `docs/CHARACTERS.md` (4 survivants uniques)
· `docs/ZONES.md` (5 zones, bestiaire) · `docs/MODES.md` (solo/coop, homelab) · `docs/TEAM.md` (rôles, Git)
· `docs/ROADMAP.md` / `docs/PLANNING.md` (jalons et tâches semaine par semaine).
