# Format des fichiers de données

Les objets (`data/items.txt`), les recettes (`data/recipes.txt`) et les ressources récoltables
(`data/resources.txt`) sont décrits dans des fichiers texte, éditables à la main sans programmer. Le
serveur et le client Unreal les lisent avec le même code (`shared/src/data/`, classes
`odyssee::data::Catalog` et `odyssee::data::ResourceCatalog`).

## Syntaxe

```ini
# Une ligne qui commence par # est un commentaire.
[item stone_axe]
name = Hache de pierre
stack = 1
category = tool
```

- Un **bloc** commence par un en-tête `[type identifiant]` : `item` dans `items.txt`, `recipe`
  dans `recipes.txt`, `resource` dans `resources.txt`. Il continue jusqu'à l'en-tête suivant.
- Chaque ligne du bloc est `clé = valeur` ; les espaces autour de la clé et de la valeur sont
  ignorés. Une valeur vide est une erreur.
- Lignes vides et commentaires `#` (en début de ligne seulement) sont ignorés : un `#` dans une
  valeur fait partie de la valeur.
- **Identifiants** (`stone_axe`) : minuscules sans accent, chiffres et `_`, commençant par une
  lettre, 48 caractères au plus. Ils sont la référence stable (sauvegardes, recettes, client) :
  ne plus les changer une fois utilisés. Les noms affichés, eux, peuvent changer librement.
- Fichier en UTF-8 (BOM toléré), fins de ligne Unix ou Windows, 1024 octets max par ligne.
- Clé inconnue, clé répétée (sauf `input` et `tool`) ou clé obligatoire absente : erreur.

## Objets (`items.txt`)

| Clé | Obligatoire | Valeur |
|---|---|---|
| `name` | oui | nom affiché, en français |
| `stack` | oui | taille de pile maximale, 1 à 9999 |
| `category` | oui | `resource`, `tool`, `weapon`, `ammo`, `station`, `consumable`, `equipment` |
| `model` | non | modèle 3D, relatif à `art/exports/`, sans extension : `outils/hache_pierre` |
| `eat` | non | consommable seulement : faim rendue, en points de % (0 à 100) |
| `drink` | non | consommable seulement : soif rendue, en points de % (0 à 100) |

Chaque objet reçoit au chargement un indice compact (`ItemId`) dans l'ordre du fichier : c'est cet
indice qui circule dans la simulation, jamais d'une partie à l'autre (utiliser l'identifiant texte
pour tout ce qui est sauvegardé).

## Recettes (`recipes.txt`)

```ini
[recipe stone_axe]
station = hand
input = stick 1
input = stone 2
output = stone_axe 1
ticks = 100
```

| Clé | Obligatoire | Valeur |
|---|---|---|
| `station` | oui | `hand` (partout), `workbench` (établi), `forge` |
| `input` | oui, 1 à 8 fois | ingrédient `identifiant quantité`, une ligne par objet différent |
| `output` | oui | résultat `identifiant quantité` (au plus la pile max de l'objet) |
| `ticks` | oui | durée de fabrication, 1 à 12000 (20 ticks = 1 s) |

Les objets cités doivent exister dans `items.txt`. Quantités de 1 à 9999.

## Ressources récoltables (`resources.txt`)

```ini
[resource tree]
name = Arbre
shape = tree
item = wood_log 1
hits = 6
hit_ticks = 15
tool = stone_axe
tool = metal_axe
regrow = 6000
abundance = 45
```

| Clé | Obligatoire | Valeur |
|---|---|---|
| `name` | oui | nom affiché, en français |
| `shape` | oui | forme provisoire tant qu'il n'y a pas de modèle : `tree`, `rock`, `bush`, `vein` |
| `item` | oui | objet gagné à **chaque coup**, `identifiant quantité` (au plus la pile max de l'objet) |
| `hits` | oui | coups avant épuisement, 1 à 1000 |
| `hit_ticks` | oui | ticks entre deux coups **avec l'outil**, 1 à 200 (20 ticks = 1 s) |
| `tool` | non, 0 à 8 fois | outil qui récolte à pleine vitesse (objet de catégorie `tool`), une ligne par outil |
| `hand` | non | `yes` (défaut) ou `no` : récoltable sans outil ? `no` exige au moins un `tool` |
| `regrow` | oui | ticks avant que la ressource épuisée repousse, entière, 1 à 1 728 000 (24 h) |
| `abundance` | oui | poids relatif au placement automatique, 0 à 1000 (0 : jamais placée d'office) |

**Règle de récolte** : avec un des outils indiqués (ou si aucun n'est indiqué), un coup toutes les
`hit_ticks` ; à la main, **deux fois plus lent** (`kBareHandSlowdown`), ou impossible si `hand = no`.
Chaque coup rapporte `item` ; après `hits` coups, la ressource est épuisée jusqu'à sa repousse.

Chaque type reçoit un indice compact dans l'ordre du fichier (128 types au plus) : il circule sur le
réseau, client et serveur doivent donc avoir le même fichier. Le serveur lit `data/` du dossier
courant (ou `--data dossier`) ; sans `resources.txt`, le monde n'a pas de ressources.

## Erreurs

Au premier problème, le chargement s'arrête et donne le fichier, la ligne et la raison, en
français : `recipes.txt:42 : objet inconnu « stone_ax » (absent de items.txt)`. Aucune donnée
invalide n'est devinée ni corrigée : à charger les données, le serveur devra refuser de démarrer.
