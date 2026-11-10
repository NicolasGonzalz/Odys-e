# Génération du monde

Outils qui construisent la **base de la carte** d'Odyssée à partir d'un seul fichier : `world_layout.json`.

| Fichier | Rôle |
|---|---|
| `world_layout.json` | **Source unique** de la géographie : zones, sous-régions, rivière, lac, chemins, lieux, boss, artefacts, cocons, brouillard toxique. Relevé sur `art/concepts/monde/carte-du-monde-plan.png` |
| `generate_terrain.py` | Génère le relief (heightmap) et ses fichiers dérivés |
| `preview.py` | Dessine l'aperçu `output/preview.png` (zones, eau, relief, lieux) |
| `output/` | Fichiers générés, versionnés via Git LFS : le coéquipier n'a pas besoin de Python |

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
