# Format des fichiers de données

Les objets (`data/items.txt`) et les recettes (`data/recipes.txt`) sont décrits dans des fichiers
texte, éditables à la main sans programmer. Le serveur et le client Unreal les lisent avec le même
code (`shared/src/data/`, classe `odyssee::data::Catalog`).

## Syntaxe

```ini
# Une ligne qui commence par # est un commentaire.
[item stone_axe]
name = Hache de pierre
stack = 1
category = tool
```

- Un **bloc** commence par un en-tête `[type identifiant]` : `item` dans `items.txt`, `recipe`
  dans `recipes.txt`. Il continue jusqu'à l'en-tête suivant.
- Chaque ligne du bloc est `clé = valeur` ; les espaces autour de la clé et de la valeur sont
  ignorés. Une valeur vide est une erreur.
- Lignes vides et commentaires `#` (en début de ligne seulement) sont ignorés : un `#` dans une
  valeur fait partie de la valeur.
- **Identifiants** (`stone_axe`) : minuscules sans accent, chiffres et `_`, commençant par une
  lettre, 48 caractères au plus. Ils sont la référence stable (sauvegardes, recettes, client) :
  ne plus les changer une fois utilisés. Les noms affichés, eux, peuvent changer librement.
- Fichier en UTF-8 (BOM toléré), fins de ligne Unix ou Windows, 1024 octets max par ligne.
- Clé inconnue, clé répétée (sauf `input`) ou clé obligatoire absente : erreur.

## Objets (`items.txt`)

| Clé | Obligatoire | Valeur |
|---|---|---|
| `name` | oui | nom affiché, en français |
| `stack` | oui | taille de pile maximale, 1 à 9999 |
| `category` | oui | `resource`, `tool`, `weapon`, `ammo`, `station`, `consumable`, `equipment` |
| `model` | non | modèle 3D, relatif à `art/exports/`, sans extension : `outils/hache_pierre` |

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

## Erreurs

Au premier problème, le chargement s'arrête et donne le fichier, la ligne et la raison, en
français : `recipes.txt:42 : objet inconnu « stone_ax » (absent de items.txt)`. Aucune donnée
invalide n'est devinée ni corrigée : à charger les données, le serveur devra refuser de démarrer.
