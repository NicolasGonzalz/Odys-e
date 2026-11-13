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
- **ECS** (maison, `server/src/ecs/`, [ADR 0004](adr/0004-ecs-maison.md)) : entités (joueurs, ressources,
  créatures, objets au sol) stockées en composants contigus ; les systèmes sont des fonctions appelées
  dans un ordre fixe à chaque tick. Les joueurs ont `PlayerControl` et `Motion` (T043) ; les ressources
  récoltables, `ResourceNode`, `Placement` et `NetworkEntity` (`server/src/resources/`, T044).
- **Area of Interest** : chaque joueur ne reçoit que les entités des chunks proches de lui.
- **Persistance** : sauvegarde périodique et à l'arrêt (SQLite), chunks modifiés uniquement.

### `client/`
- **Réseau** : envoi des inputs horodatés par tick, réception des snapshots.
- **Prédiction & réconciliation** : le joueur local est simulé immédiatement ; à la réception d'un état serveur,
  on corrige puis on rejoue les inputs non encore confirmés.
- **Interpolation** : les autres entités sont affichées légèrement dans le passé (~100 ms) entre deux snapshots.
- **Rendu** : raylib, maillages de chunks générés côté client, UI d'inventaire/artisanat.

## Pipeline de la carte (monde fait main)

Le serveur ne dépend pas d'Unreal ; il a pourtant besoin de connaître la carte. La carte a donc **une seule source**
et deux formats dérivés :

```
Blender (props, ruines, modèles)  ─┐
                                   ├─▶  Éditeur Unreal : carte (terrain, décors, acteurs marqueurs)
Figma (plans de zones, concepts) ──┘                    │
                                                        ├─▶ client : niveau Unreal (rendu)
                                                        └─▶ outil d'export ─▶ fichier monde serveur
                                                              (heightmap, volumes de collision,
                                                               spawns de ressources/créatures,
                                                               indices, boss, artefacts, déclencheurs de zone)
```

Les **marqueurs** (acteurs Unreal dédiés, définis en C++ dans le plugin : `ResourceSpawn`, `ClueSpot`, `BossArena`, `ZoneGate`…) portent les données
de gameplay. Le serveur charge le fichier exporté au démarrage ; ce fichier est versionné avec le reste du projet.

En attendant les marqueurs Unreal, le fichier monde est écrit par `tools/worldgen/export_world.py` à partir des mêmes
données que le niveau (relief, plan du blockout) ; son format est décrit dans [WORLD_FORMAT.md](WORLD_FORMAT.md).

## Rendu & performances (client)

**Objectif : 60 FPS sur un PC moyen**, avec des préréglages graphiques (bas / moyen / haut / ultra) pour les portables.

| Technique | Rôle |
|---|---|
| Frustum culling | Ne pas dessiner ce qui est hors du champ de vision (automatique dans Unreal) |
| Occlusion culling | Ne pas dessiner ce qui est caché derrière un mur (automatique, à vérifier en ville) |
| LOD | Modèles simplifiés au loin |
| Distances de visibilité | Herbe, cailloux et détails disparaissent au loin |
| Instanced meshes (outil Foliage) | Végétation dessinée par milliers d'instances |
| World Partition / HLOD | Découpage du niveau en cellules chargées selon la distance, versions simplifiées au loin |
| Streaming par distance | Les chunks se chargent et se déchargent selon la **distance** (jamais selon la direction du regard, pour éviter les saccades) |

Côté serveur, la zone d'intérêt se base aussi sur la **distance**, pas sur le regard : ce qui est dans le dos du joueur
doit continuer d'exister (un loup qui approche par derrière).

**Un monde vivant :** chaque zone doit avoir des éléments d'ambiance en plus des ressources et des ennemis :
végétation animée par le vent, oiseaux, insectes, particules (pollen, lucioles, spores), sons d'ambiance, météo.

## Simulation partagée (`shared/sim`, T029)

`sim::step(état, input, terrain)` fait avancer un joueur d'un tick : marche, course (endurance), accroupi,
saut, gravité, marches de 25 cm, murs et pentes de plus de 45°. Le serveur l'applique pour faire autorité,
le client pour prédire (puis réconcilier) : **le résultat doit être identique au bit près** sur le serveur
(Linux, GCC) et le client (Windows, MSVC dans Unreal).

- **Que des entiers** : positions en 1/256 m (celles du réseau), vitesses en 1/256 m par tick, endurance
  de 0 à 1000. Pas de `float` : leurs résultats varient d'un compilateur à l'autre (contractions FMA,
  options d'optimisation). Sinus et cosinus viennent d'une table figée (`sim/FixedMath`), les racines d'un
  calcul entier.
- **Fonction pure** : aucun état caché, aucun hasard, aucune horloge. L'input est celui envoyé, déjà
  quantifié.
- **Terrain abstrait** (`sim::Terrain`) : le sol **sous une hauteur donnée** (`groundBelow` : sous le
  relief, on marche sur le sol du bunker ou de la grotte) et les volumes solides (`isSolid` : murs et
  plafonds). La vraie carte (`world::WorldTerrain`, T041) répond en entiers, identiquement des deux
  côtés : relief interpolé sur les triangles du Landscape, cases percées, volumes tournés (pavés et
  cylindres) élargis du rayon du joueur, rangés dans une grille de 16 m.
- Réglages provisoires dans `sim::MovementTuning` (vitesses de docs/GAMEPLAY.md), à ajuster en jouant.
- **Preuve du déterminisme (T030, `tests/test_determinism.cpp`)** : une partie scriptée de 2000 ticks
  (collines, falaises, course jusqu'à l'épuisement, sauts) produit une **empreinte de référence** figée.
  La CI la vérifie sous Linux (GCC, sanitizers) et sous Windows (MSVC, le compilateur d'Unreal) : si
  les deux passent, client et serveur calculent la même chose. Un changement voulu des règles ou des
  réglages change l'empreinte : la mettre à jour dans le même commit. D'autres tests vérifient que
  repartir d'un snapshot (`sim::restore`) puis rejouer retombe exactement sur l'état du serveur, et
  que le serveur (inputs en lots redondants) et la prédiction du client arrivent au même état.

### Dans Unreal (T016)

- **`UOdysseeMovementComponent`** (plugin OdysseeNet) déplace le personnage avec `sim::step`, à 20 pas
  par seconde comme le serveur, et interpole l'affichage entre deux pas. Le `CharacterMovementComponent`
  du modèle First Person est coupé au lancement ; la capsule ne sert plus qu'aux requêtes (tirs, traces).
- **`UOdysseeWorldSubsystem`** charge le même fichier monde que le serveur : `<Projet>/World/odyssee.world`
  (jeu empaqueté) ou `tools/worldgen/output/odyssee.world` du dépôt (développement). Sans lui : sol plat.
- **Repère** : identique à celui de la simulation (X est, Y sud, Z haut, cap de +X vers +Y) ; seule
  l'échelle change (1/256 m ↔ cm, `OdysseeUnits.h`). La position de simulation est celle des pieds.
- Le personnage transmet ses entrées (déplacement, saut, et `SprintAction` / `CrouchAction` si elles sont
  renseignées) ; le cap vient de la rotation du contrôleur. Les animations et le HUD lisent l'état par
  `GetVelocity`, `IsGrounded`, `IsCrouched`, `GetStamina`…
- **En ligne (T027)** : `UOdysseeNetSubsystem` (sous-système du GameInstance, il survit aux changements
  de carte) tient le `client::ServerLink`. À chaque image, il envoie et reçoit, et place les autres joueurs
  (`AOdysseeRemotePlayer`, interpolés ; le Blueprint utilisé se choisit dans Paramètres du projet →
  Odyssee → Réseau). Connecté, le composant de mouvement **prédit** avec `client::Prediction` (comme le
  client de debug) : il apparaît où le serveur le dit, envoie ses inputs et se recale sur les snapshots.
- **Ressources (T044)** : `UOdysseeCatalogSubsystem` charge aussi `resources.txt` (même dossier que
  `items.txt`, même fichier que le serveur : le type circule par son rang). `UOdysseeResourceSync`
  (tenu par `UOdysseeNetSubsystem`) crée, met à jour et détruit un `AOdysseeResourceActor` par entité
  `Resource` du **dernier** snapshot, sans interpolation (elles sont fixes) ; tout est retiré à la
  déconnexion. Forme provisoire selon `shape` (formes de base de l'Engine, plus petite et pâle une fois
  épuisée) ; un Blueprint enfant, choisi dans Paramètres du projet → Odyssee → Réseau → Ressources, lit
  `ResourceKey` / `bDepleted` et réagit à `OnResourceStateChanged` pour montrer de vrais modèles.
- **Récolte** : `PrimaryAction` (clic gauche tenu, facultative) envoie `InputButton::Primary` ; le
  serveur décide de tout (ressource visée, cadence, gain), l'inventaire revient par le canal fiable.
- **Écrans** : `AOdyssePlayerController` affiche le menu principal au lancement (Multijoueur : adresse,
  pseudo, code ; Solo → Nouvelle partie : hors ligne), le HUD en partie (endurance, ping, pertes) et le
  menu pause sur Échap (Quitter = se déconnecter et revenir au menu).

## Modèle de threads (cible)

- **Serveur** : un thread de simulation (pas de verrous dans la logique de jeu), un thread réseau I/O,
  un thread de persistance alimenté par une file. La génération de chunks pourra passer sur un pool de workers.
- **Client** : thread de jeu Unreal (entrées + simulation locale), thread réseau dans le plugin OdysseeNet.

## Conventions de code

- C++20, warnings stricts (`-Wall -Wextra -Wconversion…`), pas d'exceptions dans la boucle de simulation.
- Namespaces : `odyssee::net`, `odyssee::sim`, `odyssee::server`, `odyssee::client`…
- Membres privés préfixés `_`, types en `PascalCase`, fonctions en `camelCase`, constantes en `kPascalCase`.
- Formatage : `.clang-format` à la racine. Code et identifiants en anglais, documentation en français.
- Les décisions structurantes sont consignées en ADR dans [`docs/adr/`](adr/).

### Modularité

1. **Un fichier = une notion.** Une classe, ou un petit groupe de fonctions qui parlent de la même chose.
   Le nom du fichier dit ce qu'il contient.
2. **Une fonction = une action.** Nommée par un verbe (`createSocket`, `bindToPort`), elle tient sur un écran
   (~25 lignes). S'il faut écrire « et » pour la décrire, on la coupe en deux. Une fonction de haut niveau
   se lit comme une liste d'étapes qui appellent des fonctions plus petites.
3. **Le code propre à une plateforme** vit dans son propre dossier (`net/posix/`, `net/win32/`) ;
   le reste du code ne voit que l'interface commune.
4. **Code aéré** : une ligne vide entre chaque étape logique ; les commentaires expliquent le **pourquoi**.
5. **Les détails internes restent cachés** : `namespace` anonyme dans le `.cpp`, ou dossier `detail/`
   s'ils servent à plusieurs fichiers.
6. **Une branche = un sujet** : on ne mélange pas une fonctionnalité et une refactorisation dans la même PR.
7. **100 caractères maximum par ligne** (comptés en caractères : les accents ne comptent qu'une fois).
