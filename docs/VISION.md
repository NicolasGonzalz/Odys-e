# Vision & game design

## Pitch

Un monde 3D low-poly partagé et persistant. On y arrive seul et démuni ; on explore, on récolte, on fabrique
des outils et des abris, on affronte l'environnement, puis on s'organise avec d'autres joueurs pour aller plus loin.
Le monde continue d'exister quand on se déconnecte : ce qu'on construit reste.

## Piliers

1. **Exploration** — un monde procédural assez varié pour donner envie d'aller voir derrière la colline.
2. **Survie & progression** — besoins vitaux, récolte, artisanat ; chaque palier d'outils ouvre de nouvelles zones et ressources.
3. **Persistance** — le monde, les constructions et les inventaires survivent aux déconnexions et aux redémarrages du serveur.
4. **Ensemble ou seul** — tout est faisable en solo, mais coopérer rend les choses plus rapides et plus ambitieuses.
5. **Réactivité** — malgré la latence, se déplacer, frapper ou récolter doit sembler instantané.

## Boucle de jeu

```
explorer → récolter → fabriquer → (construire / s'équiper) → survivre → explorer plus loin
```

- **Boucle courte (secondes)** : se déplacer, frapper un arbre, ramasser une ressource.
- **Boucle moyenne (minutes)** : fabriquer un outil, cuisiner, poser un abri, gérer faim/soif/santé.
- **Boucle longue (heures)** : débloquer des paliers de technologie, bâtir une base, explorer des biomes hostiles.

## Systèmes de jeu (périmètre cible)

| Système | Description | Priorité |
|---|---|---|
| Déplacement | Marche, course, saut, collisions avec le terrain | MVP |
| Monde | Terrain procédural par chunks, biomes, végétation | MVP |
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

## Direction artistique

Low-poly stylisé, couleurs franches, flat shading, éclairage simple mais lisible. Ce style est rapide à produire seul,
léger à afficher et vieillit bien. Les assets sont soit faits maison (Blender), soit sous licence libre (CC0).

## Public & plateforme

PC (Linux en priorité, Windows ensuite). Sessions de 30 min à plusieurs heures, de 1 à ~50 joueurs simultanés par monde.
