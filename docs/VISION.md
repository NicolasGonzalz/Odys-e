# Vision & game design

## Pitch

Un monde 3D low-poly partagé et persistant. On y arrive seul et démuni ; on explore, on récolte, on fabrique
des outils et des abris, on affronte l'environnement, puis on s'organise avec d'autres joueurs pour aller plus loin.
Le monde continue d'exister quand on se déconnecte : ce qu'on construit reste.

## Identité

| Axe | Choix |
|---|---|
| **Univers** | Post-apocalyptique *doux* : la nature a repris un monde abandonné. On fouille les vestiges, on reconstruit. Contemplatif plutôt que sombre. |
| **Ton** | Survie **équilibrée** : vrais dangers et besoins vitaux, mais on ne perd pas tout en mourant. Difficulté réglable par monde. |
| **Monde** | **Carte faite main**, conçue zone par zone, avec des lieux pensés pour raconter quelque chose. |
| **Fil conducteur** | Un **mystère** : que s'est-il passé ? Il se découvre par des **indices dans le monde** (ruines, journaux, inscriptions, objets), **sans cinématiques**. |
| **Progression** | Des **jalons** donnent une direction : **zones** à débloquer, **boss**, **artefacts** qui ouvrent de nouvelles capacités et font avancer l'enquête. |

### Narration environnementale

- Le récit est **éclaté** : chaque ruine, journal ou objet apporte un fragment ; le joueur reconstitue l'histoire.
- Les indices découverts sont gardés dans un **journal du joueur**, consultable à tout moment.
- En coop, chacun peut trouver des indices différents : on enquête ensemble.
- Les artefacts relient les deux axes : ils sont à la fois des **outils de progression** (accès à une zone, nouvelle capacité)
  et des **pièces du mystère**.

### Structure du monde (principe)

Des zones de plus en plus dangereuses autour d'une zone de départ calme. Chaque zone a ses ressources, sa faune,
ses ruines à explorer et un gardien (boss) ou une énigme qui protège un artefact. L'artefact obtenu ouvre l'accès
à la zone suivante et révèle une partie du mystère.

## Piliers

1. **Exploration & mystère** — un monde fait main où chaque lieu a quelque chose à raconter et donne envie d'aller voir derrière la colline.
2. **Survie & progression** — besoins vitaux, récolte, artisanat ; chaque palier d'outils ouvre de nouvelles zones et ressources.
3. **Persistance** — le monde, les constructions et les inventaires survivent aux déconnexions et aux redémarrages du serveur.
4. **Ensemble ou seul** — tout est faisable en solo, mais coopérer rend les choses plus rapides et plus ambitieuses.
5. **Réactivité** — malgré la latence, se déplacer, frapper ou récolter doit sembler instantané.

## Boucle de jeu

```
explorer → récolter → fabriquer → (construire / s'équiper) → survivre → explorer plus loin
                                                         ↘ indices, boss, artefacts → mystère
```

- **Boucle courte (secondes)** : se déplacer, frapper un arbre, ramasser une ressource.
- **Boucle moyenne (minutes)** : fabriquer un outil, cuisiner, poser un abri, gérer faim/soif/santé.
- **Boucle longue (heures)** : débloquer des paliers de technologie, bâtir une base, vaincre un gardien, obtenir un artefact, ouvrir une nouvelle zone, avancer dans le mystère.

## Systèmes de jeu (périmètre cible)

| Système | Description | Priorité |
|---|---|---|
| Déplacement | Marche, course, saut, collisions avec le terrain | MVP |
| Monde | Carte faite main découpée en chunks, zones, végétation | MVP |
| Indices & journal | Objets lisibles, journal du joueur, progression du mystère | MVP |
| Artefacts & zones | Artefacts qui débloquent zones et capacités | v1 |
| Boss | Gardiens de zone, combats scriptés | v1 |
| Récolte | Ressources (bois, pierre, fibres…) avec repousse | MVP |
| Inventaire | Emplacements, piles, glisser-déposer | MVP |
| Artisanat | Recettes, établis, paliers | MVP |
| Survie | Santé, faim, soif, endurance | MVP |
| Persistance | Sauvegarde monde + joueurs | MVP |
| Construction | Pose de pièces sur grille (murs, sols, portes) | v1 |
| Créatures | IA de faune passive/hostile, combat | v1 |
| Cycle jour/nuit & météo | Impact sur la visibilité et la survie | v1 |
| Social | Chat, groupes, partage de base | v1 |
| Économie / échanges | Commerce entre joueurs | plus tard |

## Hors périmètre (pour l'instant)

- Mondes à plusieurs serveurs / sharding : un serveur = un monde.
- Monétisation, comptes centralisés, matchmaking.
- PvP compétitif équilibré.

## Modes de jeu

**Solo**, **duo** et **coop** depuis un même menu, plus un **monde persistant** hébergé sur un serveur dédié.
Un monde solo peut être ouvert aux amis à tout moment. Détails : [MODES.md](MODES.md).

## Direction artistique

Low-poly stylisé, couleurs franches, flat shading, éclairage simple mais lisible. Ce style est rapide à produire seul,
léger à afficher et vieillit bien. Les assets sont soit faits maison, soit sous licence libre (CC0).

Chaîne de production : **Figma** (concepts de personnages, palettes, maquettes d'UI, icônes) → **Blender** (modèles, rig, animations) → export **glTF (.glb)** → client.

## Public & plateforme

PC (Linux en priorité, Windows ensuite). Sessions de 30 min à plusieurs heures, de 1 à ~50 joueurs simultanés par monde.
