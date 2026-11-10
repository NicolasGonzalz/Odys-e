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

## Importer le relief dans Unreal (5.6)

1. Créer un niveau vide de type **Open World** (World Partition), par ex. `Lvl_Monde`.
2. Mode **Landscape** → **Manage** → **New** → **Import from File** → `tools/worldgen/output/heightmap.png`.
3. Réglages :
   - **Location** : 0, 0, 0
   - **Scale** : X = 100, Y = 100, **Z = 200** (1 m par point, hauteurs de −512 à +512 m)
   - **Section Size** : 63 × 63 quads, **Sections per Component** : 1
   - **Number of Components** : 64 × 48 → **Overall Resolution 4033 × 3025** (doit correspondre)
4. **Import**. Ajouter ensuite un grand plan d'eau à Z = 0 pour la rivière, le lac et les mares.

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

