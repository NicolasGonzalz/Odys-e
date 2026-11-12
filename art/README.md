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

## Modèles 3D générés par script (`blender/`)

Les objets et décors simples sont **construits par script** dans Blender (style low-poly des planches),
pour avancer sans attendre la modélisation à la main. Les personnages et créatures restent à faire à la main :
les versions scriptées ne sont que des ébauches.

```sh
blender -b --python art/blender/build.py -- hache        # un modèle
blender -b --python art/blender/build.py -- --all        # tous
```

| Dossier | Contenu |
|---|---|
| `blender/modeles/<catégorie>/<nom>.py` | Un fichier = un modèle (fonction `build()`), d'après les planches de `concepts/` |
| `blender/lowpoly/` | Outils communs : matériaux, formes, **formes organiques** (tubes courbés, volumes par tranches), assemblage, export, aperçu |
| `blender/references/` | L'image de concept de chaque modèle, découpée dans les planches (`crop.py`), et `compare.py` qui met concept et rendu côte à côte |
| `blender/sources/<catégorie>/<nom>.blend` | Le modèle, **retouchable à la main** dans Blender |
| `exports/<catégorie>/<nom>.fbx` | Le modèle prêt pour Unreal (mètres → centimètres, pivot au pied) |
| `blender/apercus/<nom>.png` | Rendu de vérification |
| `exports/icones/<nom>.png` | Icône d'inventaire (PNG transparent 256 px), pour les modèles qui déclarent `ICON = True` |

Options d'un modèle (variables de son fichier) : `VIEW` (angle de l'aperçu), `ICON = True` (objet
d'inventaire), `EXPORT = False` (repère gardé dans Blender, sans .fbx), `OUTLINE = False` (sans contour noir).

| Modèle | Statut |
|---|---|
| `outils/hache_metal`, `outils/pioche_metal` | ✅ d'après la planche (tête de hache redessinée) ; bois retravaillé (fil, nœud, manche ovale) |
| `outils/hache_pierre`, `outils/pioche_pierre` | ✅ sans concept, style de la plaine ; bois retravaillé |
| `armes/arbalete` | 🟡 d'après la planche, ajouts de design à valider (arc en lame de ressort, étrier) |
| `ressources/buche`, `ressources/pierre`, `ressources/minerai` | ✅ objets au sol et d'inventaire, + icône |
| `outils/jumelles`, `outils/lampe`, `outils/gps`, `outils/pied_de_biche` | 🟡 d'après les ambiances (Site des Portes, Ville engloutie), à valider |
| `armes/arc`, `armes/lance`, `armes/pistolet`, `armes/fusil_pompe` | 🟡 d'après les ambiances (Abri, Lac gelé, Cœur, hangar, Grande Porte), à valider |
| `armes/fleche` | 🟡 flèche universelle : même munition pour l'arc et l'arbalète (`lowpoly/arrow.py`) |
| `objets/pile` | 🟡 pile récupérée, énergie de la lampe et du GPS |
| `objets/carte` | 🟡 carte vierge : seule la zone de départ est dessinée, le reste se dévoile en jeu (docs/GAMEPLAY.md § 3) |
| `outils/marteau` | 🟡 marteau de charpentier, pour la construction (bac à sable, plus tard) |
| `postes/etabli`, `postes/forge`, `postes/table_cartographie` | 🟡 postes d'artisanat (docs/GAMEPLAY.md § 2 et 3) : établi d'après le concept, forge et table dans le même style |
| `postes/coffre`, `postes/feu_de_camp` | 🟡 d'après le concept des objets interactifs ; animations (couvercle, flammes) plus tard |
| `ressources/planches`, `ressources/baton`, `ressources/lingot` | 🟡 matériaux transformés : pile de 3 planches, branche, lingot fondu main |
| `munitions/munitions_pistolet`, `munitions/munitions_fusil` | 🟡 petits tas de 3 cartouches (`lowpoly/cartridge.py`), contour plus fin |
| `reference/squelette` | Repère d'échelle 1,75 m (main droite tendue), **Blender seulement**, jamais exporté |
| `vegetation/sapin` | ✅ fidèle à la planche (40 faces) |
| `animaux/chien` | ✅ proche du concept : proportions, couleurs, pose (442 faces) ; pelage hirsute et gueule à affiner |
| `animaux/loup` | 🟡 ancienne ébauche (gris, sans concept) : à remplacer par le loup végétal de Gaïa |

Pour vérifier un modèle : `python3 art/blender/references/compare.py <nom>` → `blender/apercus/<nom>_comparaison.png`.

