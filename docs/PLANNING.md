# Planning

> Fichier généré par `scripts/planning.py render` depuis `planning/tasks.tsv` et `planning/milestones.tsv`.
> Pour modifier le planning : éditer les TSV, puis relancer le script.

- **Début :** lundi 16 nov. 2026 · **Fin prévue du MVP :** semaine 23 (25 avr. 2027)
- **Rythme :** ~12 h/semaine (10–15 h), objectifs fixés semaine par semaine
- **Charge totale :** 256 h, 57 tâches
- **Suivi au quotidien :** GitHub Issues + Project (une issue par tâche)

## Règles du jeu

1. **Lundi** : on relit l'objectif de la semaine et on passe ses issues en « En cours ».
2. **Une tâche = une branche = une PR**, nommée d'après l'ID (`t021-paquet-header`).
3. **Dimanche** : bilan rapide. Ce qui n'est pas fini glisse à la semaine suivante, et on décale le planning plutôt que d'empiler.
4. **Fin de jalon** : démo + post d'avancement public.
5. Une tâche qui dépasse 2× son estimation est redécoupée.

## Jalons

| Jalon | Échéance | Objectif |
|---|---|---|
| **M0 — Conception & fondations** | 29 nov. 2026 | Documentation de conception complète (mystère, survivants, zones, style) et fondations techniques (logger, CI). |
| **M0.5 — Spike Godot** | 18 déc. 2026 | Valider ou rejeter l'ADR 0002 : GDExtension + shared, personnage .glb animé, export Windows. |
| **M1 — Premier contact réseau** | 17 janv. 2027 | Couche UDP maison, handshake, plusieurs clients connectés au serveur du homelab. |
| **M2 — Mouvement fluide** | 7 févr. 2027 | Simulation partagée, prédiction, réconciliation, interpolation. |
| **M3 — Zone de départ** | 28 févr. 2027 | Zone de départ faite main, export de la carte, collisions et zone d'intérêt côté serveur. |
| **M4 — Récolter, fabriquer, enquêter** | 28 mars 2027 | ECS, ressources, inventaire, artisanat, premiers indices et journal. |
| **M5 — Survivre & persister (MVP)** | 25 avr. 2027 | Survie, mort avec sac au sol, persistance, menu solo/coop, codes d'invitation. |

## Semaine par semaine

### Semaine 1 — du 16 nov. 2026 · 12 h · M0

| | ID | Tâche | h | Fini quand |
|---|---|---|---|---|
| ☐ | T001 | Mystère : trame et chronologie de la catastrophe | 4 | docs/LORE.md décrit ce qui s'est passé, la chronologie et la vérité finale que le joueur reconstitue. |
| ☐ | T002 | Fiches des survivants prédéfinis | 3 | 2 à 4 fiches (nom, passé, lien avec le mystère, traits visuels) dans docs/CHARACTERS.md ; décision sur « deux joueurs, même survivant ». |
| ☐ | T003 | Règles de progression en coop | 2 | docs/VISION.md précise si artefacts, zones et journal sont partagés en coop. |
| ☐ | T004 | Logger commun | 2 | Module odyssee::log (niveaux, horodatage) utilisé par le serveur ; testé. |
| ☐ | T005 | Vérifier la CI et le badge | 1 | CI verte sur main, badge dans le README. |

### Semaine 2 — du 23 nov. 2026 · 12 h · M0

| | ID | Tâche | h | Fini quand |
|---|---|---|---|---|
| ☐ | T006 | Plan des zones (Figma) | 4 | Carte macro des zones : départ, zones dangereuses, emplacements des boss et artefacts. |
| ☐ | T007 | Guide de style visuel (Figma) | 3 | Moodboard, palette par zone, références de silhouettes low-poly. |
| ☐ | T008 | Construction, jour/nuit et météo | 2 | Choix documentés : type de construction, durée d'une journée, impact de la nuit. |
| ☐ | T009 | Fiche détaillée de chaque zone | 3 | Pour chaque zone : ambiance, ressources, faune, ruines, indices, gardien, artefact. |

### Semaine 3 — du 30 nov. 2026 · 12 h · M0.5

| | ID | Tâche | h | Fini quand |
|---|---|---|---|---|
| ☐ | T010 | Installer Godot 4 et godot-cpp | 3 | Projet Godot dans client-godot/, godot-cpp compilé, extension « hello » chargée. |
| ☐ | T011 | GDExtension embarquant odyssee_shared | 4 | Une méthode C++ appelée depuis GDScript utilise ByteWriter/ByteReader de shared. |
| ☐ | T012 | Scène test + contrôleur 1re personne | 3 | Déplacement, saut, souris, sur un sol de test. |
| ☐ | T013 | Blockout du premier survivant | 2 | Silhouette low-poly validée par rapport au concept Figma. |

### Semaine 4 — du 7 déc. 2026 · 12 h · M0.5

| | ID | Tâche | h | Fini quand |
|---|---|---|---|---|
| ☐ | T014 | Modèle, rig et bras FPS du survivant 1 | 5 | Corps entier + bras FPS riggés, animations idle et marche. |
| ☐ | T015 | Import .glb animé dans Godot | 3 | Le survivant animé s'affiche ; bras FPS visibles en vue subjective. |
| ☐ | T016 | Build Windows de l'extension + export | 4 | L'export Windows se lance sur un PC Windows (ou Wine) et charge l'extension. |

### Semaine 5 — du 14 déc. 2026 · 12 h · M0.5, M1

| | ID | Tâche | h | Fini quand |
|---|---|---|---|---|
| ☐ | T017 | Bilan du spike et décision ADR 0002 | 2 | ADR 0002 passée à « accepté » ou « rejeté », avec les raisons. |
| ☐ | T018 | Abstraction socket UDP (POSIX) | 4 | Socket UDP non bloquante, envoi/réception, testée en local. |
| ☐ | T019 | Implémentation Winsock | 3 | Même API sous Windows ; compile dans la CI Windows. |
| ☐ | T020 | Job CI Windows | 3 | La CI compile shared + serveur + tests sous Windows. |

### Semaine 6 — du 21 déc. 2026 · 6 h · M1

| | ID | Tâche | h | Fini quand |
|---|---|---|---|---|
| ☐ | T021 | En-tête de paquet, séquences et acks | 6 | Header (protocole, seq, ack, ack_bits) + comparaison modulaire, tests unitaires. |

### Semaine 7 — du 28 déc. 2026 · 6 h · M1

| | ID | Tâche | h | Fini quand |
|---|---|---|---|---|
| ☐ | T022 | Canal fiable et retransmission | 6 | Messages fiables livrés malgré 20 % de perte simulée en test. |

### Semaine 8 — du 4 janv. 2027 · 12 h · M1

| | ID | Tâche | h | Fini quand |
|---|---|---|---|---|
| ☐ | T023 | Handshake avec challenge et timeouts | 6 | Connexion, refus de version, timeout 10 s, déconnexion explicite. |
| ☐ | T024 | Messages du protocole (inputs, état) | 4 | Types de messages sérialisés et testés. |
| ☐ | T025 | Simulateur de latence et de perte | 2 | Latence, gigue, perte et duplication configurables. |

### Semaine 9 — du 11 janv. 2027 · 12 h · M1

| | ID | Tâche | h | Fini quand |
|---|---|---|---|---|
| ☐ | T026 | Sessions et diffusion de l'état | 5 | Le serveur gère N joueurs et diffuse leurs positions à chaque tick. |
| ☐ | T027 | Client Godot connecté au serveur | 5 | Connexion depuis Godot, affichage des autres joueurs. |
| ☐ | T028 | Déploiement du serveur sur le homelab | 2 | Conteneur LXC, service systemd, odyssee.lan, Uptime Kuma. |

### Semaine 10 — du 18 janv. 2027 · 12 h · M2

| | ID | Tâche | h | Fini quand |
|---|---|---|---|---|
| ☐ | T029 | Simulation de mouvement partagée | 5 | shared/sim : mouvement + collisions de base, utilisé par client et serveur. |
| ☐ | T030 | Tests de déterminisme | 3 | Même suite d'inputs = même état, client et serveur. |
| ☐ | T031 | Prédiction côté client | 4 | Le joueur local bouge sans attendre le serveur. |

### Semaine 11 — du 25 janv. 2027 · 12 h · M2

| | ID | Tâche | h | Fini quand |
|---|---|---|---|---|
| ☐ | T032 | Réconciliation | 5 | Correction + rejeu des inputs, lissage des petites erreurs. |
| ☐ | T033 | Interpolation des entités distantes | 5 | Les autres joueurs bougent sans saccades à 150 ms + 5 % de perte. |
| ☐ | T034 | HUD de debug réseau | 2 | RTT, perte, ko/s, corrections affichés à l'écran. |

### Semaine 12 — du 1 févr. 2027 · 4 h · M2

| | ID | Tâche | h | Fini quand |
|---|---|---|---|---|
| ☐ | T035 | Semaine tampon + post d'avancement | 4 | Retard rattrapé, post public M1/M2 publié. |

### Semaine 13 — du 8 févr. 2027 · 12 h · M3

| | ID | Tâche | h | Fini quand |
|---|---|---|---|---|
| ☐ | T036 | Terrain de la zone de départ | 6 | Relief, chemins et zones de la zone de départ dans Godot. |
| ☐ | T037 | Kit de ruines modulaires | 6 | Murs, sols, débris, végétation low-poly exportés en .glb. |

### Semaine 14 — du 15 févr. 2027 · 12 h · M3

| | ID | Tâche | h | Fini quand |
|---|---|---|---|---|
| ☐ | T038 | Marqueurs de gameplay | 4 | Nœuds ResourceSpawn, ClueSpot, BossArena, ZoneGate utilisables dans l'éditeur. |
| ☐ | T039 | Outil d'export de la carte | 6 | La scène Godot produit un fichier monde serveur (heightmap, collisions, marqueurs). |
| ☐ | T040 | Format du fichier monde | 2 | Format documenté et versionné. |

### Semaine 15 — du 22 févr. 2027 · 12 h · M3

| | ID | Tâche | h | Fini quand |
|---|---|---|---|---|
| ☐ | T041 | Chargement de la carte et collisions serveur | 8 | Le serveur charge la carte par chunks et applique les collisions du terrain. |
| ☐ | T042 | Zone d'intérêt (AOI) | 4 | Chaque joueur ne reçoit que les entités proches. |

### Semaine 16 — du 1 mars 2027 · 12 h · M4

| | ID | Tâche | h | Fini quand |
|---|---|---|---|---|
| ☐ | T043 | ECS serveur | 6 | Entités et composants, système de mise à jour par tick. |
| ☐ | T044 | Ressources récoltables avec repousse | 6 | Arbres et rochers récoltables, repousse côté serveur. |

### Semaine 17 — du 8 mars 2027 · 12 h · M4

| | ID | Tâche | h | Fini quand |
|---|---|---|---|---|
| ☐ | T045 | Inventaire synchronisé | 6 | Inventaire serveur, mis à jour via le canal fiable. |
| ☐ | T046 | Objets et recettes data-driven | 6 | Objets et recettes décrits dans des fichiers de données. |

### Semaine 18 — du 15 mars 2027 · 12 h · M4

| | ID | Tâche | h | Fini quand |
|---|---|---|---|---|
| ☐ | T047 | Artisanat | 4 | Fabriquer un objet consomme les ressources, validé par le serveur. |
| ☐ | T048 | UI d'inventaire et d'artisanat | 8 | Maquettes Figma intégrées dans Godot, glisser-déposer. |

### Semaine 19 — du 22 mars 2027 · 12 h · M4

| | ID | Tâche | h | Fini quand |
|---|---|---|---|---|
| ☐ | T049 | Indices lisibles et journal du joueur | 8 | Lire un journal ou une inscription l'ajoute au journal du joueur. |
| ☐ | T050 | Tampon + post d'avancement M3/M4 | 4 | Retard rattrapé, post publié. |

### Semaine 20 — du 29 mars 2027 · 12 h · M5

| | ID | Tâche | h | Fini quand |
|---|---|---|---|---|
| ☐ | T051 | Santé, faim, soif, endurance | 6 | Stats simulées par le serveur, HUD côté client. |
| ☐ | T052 | Mort, sac au sol et réapparition | 6 | Inventaire laissé sur place, récupérable ; réapparition au point de repos. |

### Semaine 21 — du 5 avr. 2027 · 12 h · M5

| | ID | Tâche | h | Fini quand |
|---|---|---|---|---|
| ☐ | T053 | Persistance SQLite | 10 | Joueurs, inventaires et modifications du monde sauvegardés et rechargés. |
| ☐ | T054 | Test de redémarrage du serveur | 2 | Arrêt/redémarrage sans perte, automatisé. |

### Semaine 22 — du 12 avr. 2027 · 12 h · M5

| | ID | Tâche | h | Fini quand |
|---|---|---|---|---|
| ☐ | T055 | Menu principal et serveur local intégré | 8 | Solo (serveur en arrière-plan), Rejoindre, Favoris, Héberger. |
| ☐ | T056 | Codes d'invitation V1 | 4 | Connexion refusée sans le bon code. |

### Semaine 23 — du 19 avr. 2027 · 12 h · M5

| | ID | Tâche | h | Fini quand |
|---|---|---|---|---|
| ☐ | T057 | Playtest, corrections et sortie du MVP | 12 | Session de jeu avec des amis sur le homelab, bugs bloquants corrigés, post MVP. |
