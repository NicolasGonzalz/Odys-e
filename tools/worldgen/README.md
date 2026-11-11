# Génération du monde

Outils qui construisent la **base de la carte** d'Odyssée à partir d'un seul fichier : `world_layout.json`.

| Fichier | Rôle |
|---|---|
| `world_layout.json` | **Source unique** de la géographie : zones, sous-régions, rivière, lac, chemins, lieux, boss, artefacts, cocons, brouillard toxique. Relevé sur `art/concepts/monde/carte-du-monde-plan.png` |
| `generate_terrain.py` | Génère le relief (heightmap) et ses fichiers dérivés |
| `preview.py` | Dessine l'aperçu `output/preview.png` (zones, eau, relief, lieux) |
| `output/` | Fichiers générés, versionnés via Git LFS : le coéquipier n'a pas besoin de Python |
| `unreal/build_map.py` | **Script Unreal** : pose la base de la carte dans le niveau (voir plus bas) |
| `unreal/elements.json` | Catalogue des éléments : forme provisoire **ou chemin du vrai modèle** |
| `unreal/scatter_rules.json` | Végétation et détails par zone : éléments, densités, bosquets, altitudes, pentes |
| `unreal/odyssee_world/` | Le code du script, une notion par fichier (plan, dispersion, ville, pose dans Unreal…) |
| `dry_run.py` | Calcule **la même génération hors d'Unreal** et la vérifie (Linux, sans éditeur) |

Repère : **x vers l'est, y vers le sud**, origine au coin nord-ouest, en mètres. Monde : 4032 × 3024 m.
Toute l'eau (rivière, lac, mares) est au **niveau 0 m** : un seul plan d'eau suffit dans Unreal.

## Régénérer

```sh
pip install -r tools/worldgen/requirements.txt   # une fois (numpy, Pillow)
python3 tools/worldgen/generate_terrain.py        # ~15 s
```

Le résultat est **déterministe** (graine fixe) : même plan → même relief.

Relief détaillé (section `relief` de `world_layout.json`) : collines dans la plaine, ravins en gradins,
buttes à deux étages, rochers affleurants tirés au hasard (graine fixe, jamais sur les chemins, lieux,
eau, grottes ni près du départ), petite falaise au-dessus de chaque entrée de grotte de montagne, puis
**facettes low-poly** : triangles plans de 8 m aux sommets décalés de ±0,5 m (chemins, lieux et eau
restent nets ; autour des grottes, le relief fin est gardé). Sans Python installé, celui d'Unreal suffit :
`pip install --target <dossier> numpy Pillow`, puis `PYTHONPATH=<dossier>`.

## Importer le relief dans Unreal (5.6)

Le niveau déjà prêt est **`Content/Odyssee/Maps/Lvl_Relief`** (relief + blockout, voir plus bas).
Pour refaire un niveau :

1. Créer un niveau vide de type **Open World** (World Partition).
2. Mode **Paysage** → **Gestion** → **Nouveau** → **Importer depuis un fichier** → `tools/worldgen/output/heightmap.png`.
3. Réglages :
   - **Emplacement** : **201600, 151200, 0** (Unreal centre le Landscape sur ce point : son coin
     nord-ouest tombe en 0, 0, comme le repère de `world_layout.json`)
   - **Échelle** : X = 100, Y = 100, **Z = 200** (1 m par point, hauteurs de −512 à +512 m)
   - **Taille de section** : 63 × 63 quads, **Sections par composant** : **2 × 2**
   - **Nombre de composants** : **32 × 24** → **résolution globale 4033 × 3025** (doit correspondre).
     Ne pas utiliser 1 section × 64 × 48 composants : 3072 composants font planter le GPU (TDR).
4. **Importer**. En World Partition, l'import peut ne remplir qu'un carré de ~2 km : charger tout le
   Landscape (Fenêtre → World Partition → sélectionner toute la carte → Charger la région), puis
   **Gestion → Importer** la même heightmap sur le Landscape existant.
5. `unreal/relief_preview.py` (Outils → Exécuter un script Python…) : couleur du sol, ciel, soleil,
   plan d'eau à Z = 0 et caméra d'ensemble.

Après chaque `generate_terrain.py`, réimporter `heightmap.png` (**Gestion → Importer**, heightmap seule).

## Blockout : spawn, lieux, grottes (`unreal/build_blockout.py`)

Blocs gris pour juger tailles et trajets, sans végétation. Relançable (remplace son blockout).

- **Centre de stase** semi-enterré : salle des capsules, couloir, sortie vers le sud ; le `PlayerStart`
  est dans la salle (Jouer = réveil dans la capsule, avec le personnage FirstPerson du template).
- **Lieux, boss, artefacts, cocons** en formes provisoires (comme `build_map.py`).
- **Grottes** (`caves` dans `world_layout.json`, dimensions dans `cave_shapes`) : tunnel, salle, deux
  lumières et un point de loot doré.
  - `montagne` : sous le relief. `generate_terrain.py` fait un replat devant l'entrée ; le tunnel et la
    salle sont en roche **sous** le Landscape. Une heightmap n'a pas de plafond : on ne creuse pas le
    relief, on **perce** le Landscape là où le tunnel sort de la pente. L'import automatique de la
    couche de visibilité est refusé avec les couches d'édition : **à peindre à la main** sur les
    repères rouges (`Odyssee/Blockout/A percer`) : Mode Paysage → Sculpture → **Visibilité**,
    taille du pinceau 250, atténuation 0 ; Maj + clic pour reboucher. Supprimer ensuite les repères.
  - `butte` : butte rocheuse creuse posée sur un terrain plat (rien à percer).
- Tout reste chargé en jeu (pas de streaming World Partition) pour marcher sur toute la carte.

`build_map.py` (végétation complète) supprime ce blockout et pose son propre bloc sur le centre de
stase : à adapter avant de le relancer sur `Lvl_Relief`.

## Vérifications faites

- Lieux à la bonne hauteur (Grande Porte à 180 m sur son plateau, départ à ~10 m).
- Col du plateau : montée continue de ~25 m à ~150 m, puis le plateau de la Porte.
- Terrain praticable (< 44°, la limite par défaut d'Unreal) : 99–100 % des zones 1, 2, 3 et 5 ;
  65 % de la montagne (vallées praticables, crêtes infranchissables).

## Poser la base de la carte (script Unreal)

**Une seule fois** :
1. `git pull`, puis ouvrir `Odysse.uproject`. Unreal propose de **recompiler** le module `Odysse` :
   accepter (nouvelle classe C++ `OdysseeScatterActor`). Les plugins **Python Editor Script Plugin** et
   **Editor Scripting Utilities** sont déjà activés dans le `.uproject` (redémarrage demandé : accepter).
2. Importer le relief dans `Lvl_Monde` (section précédente).

**À chaque génération** :
1. Ouvrir `Lvl_Monde`.
2. **Tools → Execute Python Script…** → `tools/worldgen/unreal/build_map.py`.
3. Une barre de progression s'affiche (~50 000 éléments, quelques minutes). Le niveau est sauvegardé à la fin.

Ce que le script pose (tout est rangé dans le dossier **Odyssee/** de l'Outliner) :

| Dossier | Contenu |
|---|---|
| `Odyssee/Vegetation/<élément>` | Arbres, buissons, herbes, rochers, roseaux, ronces, immeubles… instanciés par carrés de 256 m |
| `Odyssee/Lieux` | Centre de stase, ferme, village, centrale, Grande Porte, Arbre-Cœur… (blocs nommés) |
| `Odyssee/Boss`, `Artefacts`, `Cocons` | Marqueurs colorés et nommés (rouge, jaune, vert lumineux) |
| `Odyssee/Joueurs` | Point d'apparition devant le centre de stase |
| `Odyssee/Ambiance` | Soleil, ciel, nuages, plan d'eau, brumes par zone, brouillard toxique en 3 niveaux |

Le script peut être **relancé sans risque** : il supprime d'abord tout ce qu'il avait posé (étiquette
`odyssee_generated`) et ne touche à rien d'autre dans le niveau.

### Remplacer une forme provisoire par le vrai modèle

1. Importer le modèle Blender dans Unreal, par ex. `/Game/Odyssee/Vegetation/SM_Chene` (pivot au pied).
2. Dans `unreal/elements.json`, remplir `"mesh": "/Game/Odyssee/Vegetation/SM_Chene"` pour `chene`.
3. Relancer `build_map.py` : tous les chênes de la carte deviennent le vrai modèle.

### Vérifier sans Unreal

```sh
python3 tools/worldgen/dry_run.py
```

Affiche le nombre d'éléments par type, vérifie les règles (rien dans l'eau ni sur les chemins, tout dans
une zone) et dessine `output/scatter_preview.png`.

### Limites connues

- Écrit **sans avoir pu être lancé dans Unreal** : la logique est testée hors d'Unreal (dry run + module
  `unreal` simulé), mais un nom de réglage de l'API peut différer en 5.6. Les réglages d'ambiance
  incertains sont « prudents » : en cas d'écart, un avertissement s'affiche dans l'Output Log et la
  génération continue. **Envoyer les avertissements et erreurs de l'Output Log** pour corriger.
- Brumes et lumière : réglages de départ, à ajuster à l'œil.
- Formes provisoires : la disposition et l'ambiance sont justes, l'apparence vient avec les vrais modèles.

