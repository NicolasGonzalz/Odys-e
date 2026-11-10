# Modèles 3D par IA (Meshy)

Transforme un objet des planches de `art/concepts/` en modèle 3D qui **ressemble à l'image**,
puis le prépare pour Unreal (taille réelle, pivot au pied, `.fbx`, aperçu).

| Fichier | Rôle |
|---|---|
| `models.json` | Liste des modèles : planche source, cadre de découpe, catégorie, hauteur réelle, pose |
| `crop.py` | Découpe un objet par image (`crops/<nom>.png`) |
| `meshy.py` | Client de l'API Meshy (image → 3D, mode low-poly) |
| `generate.py` | Enchaîne découpe → Meshy → préparation dans Blender |
| `../blender/import_ia.py` | Mise à l'échelle, pivot, export `.fbx` / `.blend`, aperçu `<nom>_ia.png` |

## Utilisation

```sh
export MESHY_API_KEY=...                         # la clé : jamais dans le dépôt
python3 art/ia3d/crop.py                         # vérifier les découpes (gratuit)
python3 art/ia3d/generate.py hache loup_gaia     # génère (consomme des crédits Meshy)
python3 art/ia3d/generate.py --skip-meshy hache  # refait seulement la préparation (gratuit)
```

Ajouter un modèle : une entrée dans `models.json` (cadre en pixels de la planche), vérifier la découpe,
puis le générer. Pour un personnage : `"pose": "a-pose"` (bras écartés, prêt pour un squelette).

## À savoir

- Meshy est **payant au crédit** (petit quota gratuit) ; vérifier ses conditions d'usage commercial.
- Les images découpées sont **envoyées à Meshy** (service externe).
- Le résultat peut demander une retouche dans Blender (le `.blend` est fourni).
