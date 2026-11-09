# ADR 0003 — Client Unreal Engine 5 + serveur C++ maison

- **Statut :** **accepté** (2026-11-09)
- **Remplace :** [ADR 0002](0002-client-godot.md) (client Godot)

## Contexte

Le projet se fait **à deux** :
- **Nicolas** : code C++, serveur, réseau, simulation, outils, infrastructure (homelab) ;
- **son coéquipier** : à l'aise sous **Unreal Engine**, il prend en charge le visuel (Figma, Blender, cartes, intégration).

C'est le coéquipier qui passera le plus de temps dans le moteur. Sa productivité y prime donc sur les avantages
théoriques de Godot (légèreté, fichiers texte, Linux). Nicolas dispose de Linux **et** de Windows (dual-boot).

## Décision

- **Client : Unreal Engine 5** (dernière version stable au lancement du spike, puis **figée** pour tout le projet).
  Version binaire du launcher Epic : **pas de compilation du moteur depuis les sources**.
- **Serveur : C++20 maison, headless**, inchangé. Unreal n'est **jamais** utilisé côté serveur.
- **Pas de réplication Unreal.** Le client parle uniquement le protocole maison ([NETWORK.md](../NETWORK.md)).
- **Pont : plugin Unreal `OdysseeNet`** (module C++) qui embarque `shared/` :
  - la couche réseau UDP maison ;
  - la simulation partagée (mouvement, collisions), pour que la prédiction côté client soit **identique** à la simulation serveur.
  - Le pion du joueur est piloté par un composant de mouvement **maison** qui appelle `odyssee::sim`,
    et non par le `CharacterMovementComponent` (dont la prédiction est liée à la réplication Unreal).
- **Client de debug raylib** (`client/`) conservé : un client léger sous Linux pour tester le serveur et le réseau
  sans lancer Unreal, et une base pour les bots de test.

```
┌──────────────────────── Client Unreal (Windows / Linux) ───────────────────────┐
│  Cartes · Blueprints · animations · UI (UMG) · audio · effets   ← coéquipier    │
│                              ▲  composants / événements                         │
│  Plugin OdysseeNet (C++) ─ odyssee_shared (net + sim) ─ socket UDP   ← Nicolas  │
└───────────────────────────────────┬─────────────────────────────────────────────┘
                                    │ protocole maison
┌───────────────────────────────────▼─────────────────────────────────────────────┐
│  Serveur C++ headless (homelab ou local en solo) ─ odyssee_shared   ← Nicolas   │
└─────────────────────────────────────────────────────────────────────────────────┘
```

## Conséquences

**Positives**
- Le coéquipier est productif immédiatement ; rendu, éclairage, végétation et outils d'édition de premier ordre.
- Le serveur reste léger, portable et entièrement maîtrisé : le défi réseau reste intact.
- Pas de serveur dédié Unreal à compiler (évite ~150 Go et des heures de build).

**Négatives / à gérer**
- **Git LFS obligatoire** pour les fichiers binaires Unreal (`.uasset`, `.umap`) et les sources d'art,
  avec **verrouillage** des fichiers (deux personnes ne peuvent pas modifier la même carte en même temps).
  Surveiller le quota LFS de GitHub ; repli possible : forge Git (Forgejo) auto-hébergée sur le homelab.
- Le plugin doit intégrer `shared/` dans le système de build d'Unreal (UBT) : compilation des sources
  dans le module, ou bibliothèque statique précompilée par CMake pour Win64 et Linux.
- L'éditeur Unreal tourne mieux sous Windows : Nicolas code le serveur sous Linux et teste le client sous Windows.
- Projet plus lourd : quelques dizaines de Go en développement, jeu final estimé à 1,5–4 Go.
- La **carte** est faite dans l'éditeur Unreal ; l'outil d'export vers le serveur devient un utilitaire d'éditeur Unreal.

## Critères du spike (M0.5)

1. Le plugin `OdysseeNet` compile sous **Windows** (et si possible Linux) et utilise `odyssee_shared`.
2. Un pion 1re personne piloté par la simulation partagée se déplace sur un terrain de test.
3. Un personnage Blender animé est importé et visible (corps + bras FPS).
4. Le projet Unreal est versionné avec Git LFS et modifié par les deux développeurs sans conflit.
5. Un build Windows packagé se lance et se connecte à un serveur Linux (dès que M1 le permet).
