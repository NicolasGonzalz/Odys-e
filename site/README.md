# Site de présentation d'Odyssée

Site public du jeu (accueil, univers, survivants), en [Astro](https://astro.build/), déployé sur Vercel.

## Règle d'or : pas de spoiler

Le site raconte **la prémisse seulement** : le réveil, le monde redevenu sauvage, les zones, les survivants.
Jamais ce qui s'est passé, les gardiens, les artefacts ni les fins (`docs/LORE.md` reste privé).
Les textes publics sont dans `src/data/` : les docs de conception ne sont jamais publiées telles quelles.

## Lancer en local

```sh
cd site
npm install
npm run dev        # http://localhost:4321, rechargé à chaque modification
npm run build      # site final dans dist/
```

## Images

Les originaux sont dans `art/concepts/` (Git LFS). Le site en garde des versions web légères dans
`src/assets/` et `public/og.jpg` (hors LFS, voir `.gitattributes`). Pour les régénérer après un
changement de concept, depuis la racine du dépôt :

```sh
python3 site/scripts/images.py
```

## Vercel

Projet relié au dépôt GitHub, réglages :

- **Root Directory** : `site` (framework détecté : Astro, sortie `dist/`) ;
- un push sur `main` déploie le site en production, une PR donne un aperçu ;
- les changements qui ne touchent pas `site/` ne relancent pas de déploiement (voir `vercel.json`).
