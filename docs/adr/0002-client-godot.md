# ADR 0002 — Client Godot 4 + serveur C++ maison

- **Statut :** **remplacé** par [ADR 0003](0003-client-unreal.md) (2026-11-09) : la répartition du travail à deux a fait pencher pour Unreal.
- **Date :** 2026-11-09
- **Remplace partiellement :** [ADR 0001](0001-stack-technique.md) (le choix de raylib pour le client)

## Contexte

Deux objectifs à concilier :

1. Garder le **défi technique** du projet : serveur autoritaire et réseau temps réel faits main.
2. Obtenir un jeu **beau, animé, avec une vraie UI**, jouable sous **Linux et Windows**, sans y passer des années seul.

Trois options ont été étudiées :

| Option | Verdict |
|---|---|
| Tout maison (C++ + raylib) | Maîtrise totale, mais rendu, animation, UI et outils sont tous à refaire : trop lent pour un jeu complet. |
| Tout Unreal | Très beau, mais lourd (>100 Go, serveur dédié = moteur compilé depuis les sources) et sa réplication intégrée fait disparaître le défi réseau. |
| **Godot 4 (client) + serveur C++ maison** | **Le compromis retenu.** |

## Décision (proposée)

- **Serveur** : C++20 pur, headless, sans dépendance à Godot. Inchangé par rapport à l'ADR 0001.
- **Client** : Godot 4 (dernière version stable), pour le rendu, les animations, l'UI, l'audio et l'export Windows/Linux.
- **Pont** : une **GDExtension C++** (godot-cpp) qui embarque `shared/` :
  - la couche réseau UDP maison (le client Godot ne parle **jamais** via le multijoueur intégré de Godot) ;
  - la simulation partagée (mouvement, collisions), pour une prédiction client **identique** à la simulation serveur.
- La logique de présentation (scènes, UI, effets) est faite côté Godot (GDScript ou C++ selon les besoins).
- Assets : Figma (2D, UI, concepts) → Blender (3D, rig, animations) → **glTF (.glb)** → Godot.

```
┌───────────────────────── Client Godot ─────────────────────────┐
│  Scènes · UI · animations · audio (GDScript / scènes .tscn)     │
│                         ▲  signaux / appels                    │
│  GDExtension C++ ─ odyssee_shared (net + sim) ─ socket UDP      │
└────────────────────────────────┬───────────────────────────────┘
                                 │ protocole maison (docs/NETWORK.md)
┌────────────────────────────────▼───────────────────────────────┐
│  Serveur C++ headless ─ odyssee_shared (net + sim) ─ monde       │
└────────────────────────────────────────────────────────────────┘
```

## Godot face à Unreal (comparaison du 2026-11-09)

Unreal a été envisagé (l'un des deux développeurs le connaît). Godot a été retenu pour :
- le **travail à deux sous Git** : scènes en texte, fusionnables, pas de Git LFS ni de verrouillage de fichiers ;
- le **développement sous Linux**, où l'éditeur Unreal est moins bien supporté ;
- le **serveur C++ maison** sur le homelab, naturel avec Godot et à contre-courant du réseau intégré d'Unreal ;
- le **style low-poly**, qui n'a pas besoin du rendu photoréaliste d'Unreal ;
- la **légèreté** : éditeur de ~150 Mo, jeu final estimé à 0,5–1,5 Go, PC modeste suffisant.

## Critères de validation du spike

Le prototype doit démontrer, en quelques jours au maximum :

1. `odyssee_shared` se compile dans une GDExtension sous **Linux et Windows**.
2. Un client Godot se connecte au serveur C++ et affiche un personnage `.glb` animé, piloté par le serveur.
3. Le même code de simulation tourne dans le client (prédiction) et dans le serveur.
4. L'export Windows fonctionne et se connecte à un serveur tournant sous Linux.

Si un critère bloque durablement, on revient à l'option « tout maison » avec raylib, dont le client prototype existe déjà.

## Conséquences

- Deux environnements : CMake (serveur, shared, tests) et Godot (client). La CI devra compiler les deux.
- Le client raylib actuel (`client/`) sert de prototype jusqu'à la validation, puis sera retiré.
- Le défi réseau et serveur reste entièrement entre nos mains.
