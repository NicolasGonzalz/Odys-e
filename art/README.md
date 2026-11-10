# Art — références visuelles

> Ces images sont des **concepts** (générés par IA à partir de nos descriptions) : elles fixent la
> **direction artistique** et servent de modèle pour Blender et Unreal. Ce ne sont **pas** des assets du jeu.
> Les vrais modèles 3D iront dans le projet Unreal (`UnrealEngine/Odysse/Content/`).

Style commun : **low-poly stylisé**, contours légèrement marqués, couleurs franches ; rouille et lierre
sur tout ce qui est humain, lumière verte des cocons et de Gaïa, brume en zone 1, violet toxique en zone 3.

## `concepts/monde/` — la carte

| Fichier | Contenu |
|---|---|
| `carte-du-monde-plan.png` | Carte de référence : 5 zones, lieux, boss, artefacts, cocons, progression conseillée. Monde ≈ 4 × 3 km, 1 chunk = 64 m |
| `carte-du-monde-illustree.jpeg` | Même carte en vue illustrée (ambiance de chaque zone) |
| `fiche-des-biomes.png` | Fiche des 5 biomes : danger, surface, ressources, faune, boss, artefact ; les 3 niveaux du brouillard toxique |

## `concepts/ambiances/` — à quoi ressemble chaque zone, en vue 1re personne

| Fichier | Contenu |
|---|---|
| `zones-1-a-5-planche-a.jpeg` | Les 5 zones (une vignette chacune, + le hangar du Site des Portes) |
| `zones-1-a-5-planche-b.jpeg` | Variante : plaine, ville engloutie inondée, terres noyées, neige, forêt du Cœur |
| `zones-1-3-5-lieux.jpeg` | Plaine, chemin de la ferme, forêt des géants, sanctuaire des cocons, terres noyées, villages sur pilotis |
| `zone-4-site-des-portes-lieux.jpeg` | Col du plateau, Grande Porte, lac gelé, hangars, flore de Gaïa, avant-poste |
| `zone-1-plaine.jpg` | L'Abri : prairie, brume, premier cocon |
| `zone-2-ville-engloutie.jpg` | La Ville engloutie |
| `zone-3-terres-noyees.jpg` | Les Terres noyées / la Centrale |

## `concepts/decors/` — éléments à modéliser (Blender)

| Fichier | Contenu |
|---|---|
| `vegetation-ruines-campement.jpg` | Chêne, sapin, buissons, herbes, fougères, rochers moussus, lierre, mur et escalier en ruine, lampadaire, rambarde, voiture rouillée, passerelle, cuve, porte blindée, feu de camp, lit, coffre, échelle, cocons |
| `flore-gaia-industriel-neige.jpg` | Souche lumineuse, plante carnivore, arbre mort, champignons, roseaux, terminal, antenne parabolique, fûts toxiques, palissade, tuyaux, rails, wagonnet, pneus, tente, lanterne, caméra, rochers enneigés |

## `concepts/creatures/`

| Fichier | Contenu |
|---|---|
| `bestiaire.jpg` | Faune revenue (chien, sangliers), créatures de Gaïa (loup végétal, insecte-feuille, plante à tentacules), cocons, Gardiens d'Odyssée (drones, robots) |

## `concepts/objets/`

| Fichier | Contenu |
|---|---|
| `objets-interactifs.jpg` | Coffres (bois, métal), établi, feu de camp, terminal, cocon, journal, sac de couchage |
| `outils-et-ressources.png` | Hache, pioche, couteaux, arbalète, gourde, torche, bûche, pierre, minerai, fibres, tissu, sac, boîte, potion, journal, badge d'accès, cocon |

## `concepts/personnages/` — les 4 survivants ([docs/CHARACTERS.md](../docs/CHARACTERS.md))

`mara-velin.jpg` · `ilian-sorel.jpg` · `nadia-okoro.jpg` · `teo-brandt.jpg`

## Règles

- Noms en **minuscules, sans espaces ni parenthèses**, mots séparés par des tirets.
- Toutes les images passent par **Git LFS** (voir `.gitattributes`).
- Une image qui change le design (nouveau lieu, nouvelle mécanique) → mettre aussi à jour la doc (`docs/ZONES.md`…).
