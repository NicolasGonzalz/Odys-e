# Fichier monde — format, version 1

Le serveur ne charge pas Unreal ; il lui faut pourtant la carte : le sol, les trous, les murs, les
points d'intérêt. C'est le rôle du **fichier monde** `tools/worldgen/output/odyssee.world`.

- **Écrit par** `tools/worldgen/export_world.py`, à lancer après `generate_terrain.py`.
- **Lu par** `odyssee::world::decodeWorldFile` (`shared/include/odyssee/world/WorldFile.hpp`) : le
  serveur pour faire autorité, et le client pour prédire. Les deux doivent voir exactement le même
  monde, sinon la prédiction diverge.
- **Une seule source** : les blocs et les trous viennent du même plan que ceux posés dans Unreal
  (`tools/worldgen/unreal/odyssee_world/blockout_plan.py`), et le relief est le même
  `heightmap.png` que celui importé dans le Landscape.
- Versionné avec Git LFS, comme les autres fichiers générés (environ 26 Mo).

## Conventions

| | |
|---|---|
| Entiers | little-endian, taille fixe |
| Positions, longueurs | entiers en **1/256 m** (comme `net::quantizePosition`) |
| Repère | x vers l'**est**, y vers le **sud**, z vers le **haut** ; origine au coin nord-ouest de la carte |
| Cap (yaw) | `u16`, un tour = 65 536, compté de +x vers +y (comme `net::quantizeYaw`) |
| Chaînes | longueur `u16` puis octets UTF-8 (comme `ByteWriter::writeString`) |

## Structure

```
en-tête          "ODYW"  version u16 = 1  nombre de sections u16
répertoire       une entrée par section : étiquette (4 octets ASCII)  position u64  taille u64
sections         leur contenu, aux positions annoncées (depuis le début du fichier)
```

- Une section dont l'étiquette est **inconnue est ignorée** : on peut en ajouter sans changer de
  version. Toute autre modification (sens d'un champ, taille d'un enregistrement) **change la
  version**, et un lecteur refuse une version qu'il ne connaît pas.
- Chaque section connue apparaît **au plus une fois**. `TERR` est obligatoire ; les autres,
  absentes, sont vides.
- Le lecteur ne suppose rien : tailles, bornes, ordre et valeurs sont vérifiés, toute incohérence
  est une erreur (`WorldFileError`).

### `TERR` — le relief

| Champ | Type | Valeur actuelle |
|---|---|---|
| quadsX, quadsY | `u32`, `u32` | 4032, 3024 cases (la grille a un point de plus dans chaque sens) |
| quadSize | `u32` | 256 (1 m) |
| chunkQuads | `u16` | 64 cases par côté de bloc |
| réservé | `u16` | 0 |
| chunksX, chunksY | `u32`, `u32` | ⌈quadsX / chunkQuads⌉, ⌈quadsY / chunkQuads⌉ : 63, 48 |
| hauteurs | `u16` × … | bloc par bloc |

- Les **blocs** (chunks) sont rangés ligne par ligne (du nord au sud, d'ouest en est). Chaque bloc
  contient `(chunkQuads + 1)²` points, ligne par ligne : il garde ses points de bord, donc il se
  suffit à lui-même (un serveur peut ne charger que les blocs utiles). Les blocs du bord qui
  dépassent la carte répètent sa dernière rangée de points.
- **Hauteur** d'un point : `z (1/256 m) = (brut − 32768) × 4`, soit des pas de 1/64 m entre
  −512 m et +512 m. C'est le codage de `heightmap.png`.
- **Surface** : chaque case est coupée en deux triangles par sa diagonale du point (col, row) au
  point (col + 1, row + 1), comme le Landscape d'Unreal. La hauteur entre les points s'interpole
  sur ces triangles (T041).

### `HOLE` — les cases percées

`count u32`, puis `count` × (`col u32`, `row u32`).

- Une case percée n'a **ni sol ni collision** : ce sont les entrées des grottes et du centre de
  stase, où le joueur passe sous le relief. Le sol y est donné par les blocs (`BOXS`).
- Une case est percée dès qu'un de ses 4 points est dans un des rectangles du plan : le Landscape
  retire tous les triangles qui touchent un point masqué.
- Triées ligne par ligne puis colonne, sans doublon (recherche par dichotomie).

### `BOXS` — les volumes de collision

`count u32`, puis `count` enregistrements de 28 octets :

| Champ | Type |
|---|---|
| centre x, y, z | `i32` × 3 |
| demi-tailles x, y, z | `u32` × 3, non nulles (axes du bloc, avant rotation) |
| cap | `u16` (rotation autour de z, autour du centre) |
| forme | `u8` : 0 = pavé, 1 = cylindre vertical (section elliptique, demi-axes x et y) |
| réservé | `u8` = 0 |

Ce sont les blocs **solides** du blockout : sols, murs, plafonds et bouchons des grottes, centre de
stase. Les blocs sans collision dans Unreal (balises, lampes) n'y sont pas. Quand les modèles
Blender remplaceront le blockout, leurs collisions simplifiées prendront la même forme.

### `MARK` — les points d'intérêt

`count u32`, puis `count` × :

| Champ | Type |
|---|---|
| type | `u8` : 0 = apparition des joueurs, 1 = lieu, 2 = entrée de grotte |
| position x, y, z | `i32` × 3 (au sol) |
| cap | `u16` |
| identifiant | chaîne (64 octets au plus), celui de `world_layout.json` |

Les marqueurs de gameplay posés dans Unreal (T038 : ressources, indices, boss, portes de zone)
s'y ajouteront comme nouveaux types, ce qui changera la version.

## Pas encore dans le fichier

- Les formes provisoires des lieux (bâtiments du village, ferme…) : ce sont des repères visuels,
  sans collision côté serveur pour l'instant.
- La végétation et les décors : pas de collision serveur prévue (on les traverse, ou ils seront
  ajoutés comme blocs le jour où ils doivent bloquer).
- L'eau : tout est au niveau 0 m (`tools/worldgen/README.md`).

## Historique

| Version | Changement |
|---|---|
| 1 | Première version : relief par blocs, cases percées, blocs de collision, marqueurs. |
