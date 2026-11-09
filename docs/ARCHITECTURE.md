# Architecture technique

## Vue d'ensemble

```
┌──────────────────────────┐        UDP         ┌────────────────────────────────┐
│          Client          │  inputs (tick n)   │        Serveur autoritaire     │
│ ─ entrée joueur          │ ─────────────────▶ │ ─ boucle à pas fixe (20 Hz)    │
│ ─ prédiction locale      │                    │ ─ validation des inputs        │
│ ─ réconciliation         │  snapshots/deltas  │ ─ simulation du monde (ECS)    │
│ ─ interpolation entités  │ ◀───────────────── │ ─ intérêt (AOI) par chunk      │
│ ─ rendu raylib           │                    │ ─ persistance (SQLite)         │
└────────────┬─────────────┘                    └───────────────┬────────────────┘
             │                                                  │
             └──────────────►  shared/  ◄───────────────────────┘
                  protocole · sérialisation · simulation déterministe · données de jeu
```

## Principes

1. **Le serveur fait autorité.** Le client n'envoie que des *intentions* (inputs) ; il ne décide jamais de l'état du monde.
   Toute donnée reçue du réseau est considérée comme hostile et validée.
2. **Simulation partagée.** Le code qui fait avancer un joueur (mouvement, collisions) vit dans `shared/` et est exécuté
   à l'identique côté client (prédiction) et côté serveur (vérité). C'est ce qui rend la réconciliation possible.
3. **Pas de temps fixe.** La simulation avance par ticks discrets de durée constante ; le rendu est découplé et interpole.
4. **Données plutôt que code.** Objets, recettes, ressources et biomes sont décrits dans des fichiers de données,
   chargés au démarrage, pas codés en dur.
5. **Serveur headless.** Le serveur ne dépend d'aucune bibliothèque graphique et tourne sur une machine sans écran.

## Modules

### `shared/` — bibliothèque commune
| Module | Rôle |
|---|---|
| `core/` | Configuration (tick rate, ports), types de base, logging |
| `net/` | Sérialisation binaire (`ByteWriter`/`ByteReader`), définition des messages, couche transport UDP |
| `sim/` | Simulation déterministe : mouvement du joueur, collisions, physique simple |
| `world/` | Chunks, chargement de la carte exportée (heightmap, collisions, points d'intérêt) |
| `game/` | Définitions de données : objets, recettes, ressources |

### `server/`
- **Boucle de jeu** : `recevoir → appliquer inputs → simuler → construire snapshots → envoyer`, à chaque tick.
- **Gestion des sessions** : handshake, version du protocole, timeouts, déconnexion.
- **ECS** : entités (joueurs, ressources, créatures, objets au sol) stockées en composants contigus.
- **Area of Interest** : chaque joueur ne reçoit que les entités des chunks proches de lui.
- **Persistance** : sauvegarde périodique et à l'arrêt (SQLite), chunks modifiés uniquement.

### `client/`
- **Réseau** : envoi des inputs horodatés par tick, réception des snapshots.
- **Prédiction & réconciliation** : le joueur local est simulé immédiatement ; à la réception d'un état serveur,
  on corrige puis on rejoue les inputs non encore confirmés.
- **Interpolation** : les autres entités sont affichées légèrement dans le passé (~100 ms) entre deux snapshots.
- **Rendu** : raylib, maillages de chunks générés côté client, UI d'inventaire/artisanat.

## Pipeline de la carte (monde fait main)

Le serveur ne dépend pas de Godot ; il a pourtant besoin de connaître la carte. La carte a donc **une seule source**
et deux formats dérivés :

```
Blender (props, ruines, modèles)  ─┐
                                   ├─▶  Éditeur Godot : scène de la carte (terrain, décors, marqueurs)
Figma (plans de zones, concepts) ──┘                    │
                                                        ├─▶ client : scène Godot (rendu)
                                                        └─▶ outil d'export ─▶ fichier monde serveur
                                                              (heightmap, volumes de collision,
                                                               spawns de ressources/créatures,
                                                               indices, boss, artefacts, déclencheurs de zone)
```

Les **marqueurs** (nœuds Godot dédiés : `ResourceSpawn`, `ClueSpot`, `BossArena`, `ZoneGate`…) portent les données
de gameplay. Le serveur charge le fichier exporté au démarrage ; ce fichier est versionné avec le reste du projet.

## Rendu & performances (client)

**Objectif : 60 FPS sur un PC moyen**, avec des préréglages graphiques (bas / moyen / haut / ultra) pour les portables.

| Technique | Rôle |
|---|---|
| Frustum culling | Ne pas dessiner ce qui est hors du champ de vision (automatique dans Godot) |
| Occlusion culling | Ne pas dessiner ce qui est caché derrière un mur (`OccluderInstance3D`), crucial en ville |
| LOD | Modèles simplifiés au loin |
| Distances de visibilité | Herbe, cailloux et détails disparaissent au loin |
| MultiMesh | Végétation dessinée par milliers d'instances en un seul appel |
| Streaming par distance | Les chunks se chargent et se déchargent selon la **distance** (jamais selon la direction du regard, pour éviter les saccades) |

Côté serveur, la zone d'intérêt se base aussi sur la **distance**, pas sur le regard : ce qui est dans le dos du joueur
doit continuer d'exister (un loup qui approche par derrière).

**Un monde vivant :** chaque zone doit avoir des éléments d'ambiance en plus des ressources et des ennemis :
végétation animée par le vent, oiseaux, insectes, particules (pollen, lucioles, spores), sons d'ambiance, météo.

## Modèle de threads (cible)

- **Serveur** : un thread de simulation (pas de verrous dans la logique de jeu), un thread réseau I/O,
  un thread de persistance alimenté par une file. La génération de chunks pourra passer sur un pool de workers.
- **Client** : thread principal Godot (entrées + rendu + simulation locale), thread réseau dans la GDExtension.

## Conventions de code

- C++20, warnings stricts (`-Wall -Wextra -Wconversion…`), pas d'exceptions dans la boucle de simulation.
- Namespaces : `odyssee::net`, `odyssee::sim`, `odyssee::server`, `odyssee::client`…
- Membres privés préfixés `_`, types en `PascalCase`, fonctions en `camelCase`, constantes en `kPascalCase`.
- Formatage : `.clang-format` à la racine. Code et identifiants en anglais, documentation en français.
- Les décisions structurantes sont consignées en ADR dans [`docs/adr/`](adr/).
