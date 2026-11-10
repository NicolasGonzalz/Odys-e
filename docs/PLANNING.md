# Planning

> Fichier généré par `scripts/planning.py render` depuis `planning/tasks.tsv` et `planning/milestones.tsv`.
> Pour modifier le planning : éditer les TSV, puis relancer le script.

- **Début :** lundi 16 nov. 2026 · **Fin prévue du MVP :** semaine 23 (25 avr. 2027)
- **Équipe :** Nicolas (code, serveur, réseau) et son coéquipier (Unreal, art, cartes) — voir [TEAM.md](TEAM.md)
- **Rythme :** ~12 h/semaine pour Nicolas (10–15 h), objectifs fixés semaine par semaine
- **Charge totale :** 355 h, 77 tâches
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
| **M0.5 — Spike Unreal** | 18 déc. 2026 | Valider l'ADR 0003 : plugin OdysseeNet + shared, pion piloté par la simulation maison, survivant animé, Git LFS à deux. |
| **M1 — Premier contact réseau** | 17 janv. 2027 | Couche UDP maison, handshake, plusieurs clients connectés au serveur du homelab. |
| **M2 — Mouvement fluide** | 7 févr. 2027 | Simulation partagée, prédiction, réconciliation, interpolation. |
| **M3 — Zone de départ** | 28 févr. 2027 | Zone de départ faite main, export de la carte, collisions et zone d'intérêt côté serveur. |
| **M4 — Récolter, fabriquer, enquêter** | 28 mars 2027 | ECS, ressources, inventaire, artisanat, premiers indices et journal. |
| **M5 — Survivre & persister (MVP)** | 25 avr. 2027 | Survie, mort avec sac au sol, persistance, menu solo/coop, codes d'invitation. |

## Semaine par semaine

### Semaine 1 — du 16 nov. 2026 · M0 · Nicolas 12 h · Coéquipier 9 h

| | ID | Qui | Tâche | h | Fini quand |
|---|---|---|---|---|---|
| ☐ | T001 | Les deux | Mystère : trame et chronologie de la catastrophe | 4 | docs/LORE.md décrit ce qui s'est passé, la chronologie et la vérité finale que le joueur reconstitue. |
| ☐ | T002 | Les deux | Fiches des survivants prédéfinis | 3 | 2 à 4 fiches (nom, passé, lien avec le mystère, traits visuels) dans docs/CHARACTERS.md ; décision sur « deux joueurs, même survivant ». |
| ☐ | T003 | Les deux | Règles de progression en coop | 2 | docs/VISION.md précise si artefacts, zones et journal sont partagés en coop. |
| ☐ | T004 | Nicolas | Logger commun | 2 | Module odyssee::log (niveaux, horodatage) utilisé par le serveur ; testé. |
| ☐ | T005 | Nicolas | Vérifier la CI et le badge | 1 | CI verte sur main, badge dans le README. |

### Semaine 2 — du 23 nov. 2026 · M0 · Nicolas 12 h · Coéquipier 19 h

| | ID | Qui | Tâche | h | Fini quand |
|---|---|---|---|---|---|
| ☐ | T006 | Coéquipier | Plan des zones (Figma) | 4 | Carte macro des 5 zones (docs/ZONES.md) : départ, zones dangereuses, emplacements des boss et artefacts. |
| ☐ | T007 | Coéquipier | Guide de style visuel (Figma) | 3 | Moodboard « retour à l'état sauvage », palette par zone, références de silhouettes low-poly. |
| ☐ | T008 | Les deux | Construction, jour/nuit et météo | 2 | Choix documentés : type de construction, durée d'une journée, impact de la nuit. |
| ☐ | T009 | Les deux | Fiche détaillée de chaque zone | 3 | Pour chaque zone : ambiance, ressources, faune, ruines, indices, gardien, artefact. |
| ☐ | T067 | Les deux | Site de présentation (Vercel) | 4 | Site public sur odyssee-sage.vercel.app : accueil, univers et survivants, sans spoiler du mystère ; images reprises de art/. |
| ☐ | T069 | Les deux | Conception des systèmes de jeu | 3 | docs/GAMEPLAY.md : déplacement, artisanat (main, établi, forge), carte, GPS, menus, notifications, animations de combat. |

### Semaine 3 — du 30 nov. 2026 · M0.5 · Nicolas 5 h · Coéquipier 8 h

| | ID | Qui | Tâche | h | Fini quand |
|---|---|---|---|---|---|
| ☐ | T010 | Coéquipier | Projet Unreal, version figée et Git LFS | 3 | Projet dans UnrealEngine/Odysse/, version d'Unreal notée dans le README, Git LFS et verrouillage fonctionnels pour les deux. |
| ☐ | T011 | Nicolas | Plugin OdysseeNet embarquant odyssee_shared | 5 | Le plugin compile sous Windows ; un Blueprint appelle une fonction C++ qui utilise ByteWriter/ByteReader de shared. |
| ☐ | T012 | Coéquipier | Pion 1re personne et terrain de test | 3 | Déplacement, saut, souris, sur un terrain de test. |
| ☐ | T013 | Coéquipier | Blockout du premier survivant | 2 | Silhouette low-poly validée par rapport au concept Figma. |

### Semaine 4 — du 7 déc. 2026 · M0.5 · Nicolas 4 h · Coéquipier 8 h

| | ID | Qui | Tâche | h | Fini quand |
|---|---|---|---|---|---|
| ☐ | T014 | Coéquipier | Modèle, rig et bras FPS du survivant 1 | 5 | Corps entier + bras FPS riggés, animations idle et marche. |
| ☐ | T015 | Coéquipier | Import du survivant animé dans Unreal | 3 | Le survivant animé s'affiche ; bras FPS visibles en vue subjective. |
| ☐ | T016 | Nicolas | Composant de mouvement maison | 4 | Le pion est déplacé par une fonction de mouvement de shared/ (et non par le CharacterMovementComponent). |

### Semaine 5 — du 14 déc. 2026 · M0.5, M1 · Nicolas 12 h · Coéquipier 7 h

| | ID | Qui | Tâche | h | Fini quand |
|---|---|---|---|---|---|
| ☐ | T017 | Les deux | Bilan du spike Unreal | 2 | Critères de l'ADR 0003 vérifiés ; problèmes et ajustements notés. |
| ☐ | T018 | Nicolas | Abstraction socket UDP (POSIX) | 4 | Socket UDP non bloquante, envoi/réception, testée en local. |
| ☐ | T019 | Nicolas | Implémentation Winsock | 3 | Même API sous Windows ; compile dans la CI Windows. |
| ☐ | T020 | Nicolas | Job CI Windows | 3 | La CI compile shared + serveur + tests sous Windows. |
| ☐ | T058 | Coéquipier | Animations de base du survivant 1 | 5 | Course, saut, frappe, récolte ; intégrées dans Unreal. |

### Semaine 6 — du 21 déc. 2026 · M1 · Nicolas 6 h · Coéquipier 5 h

| | ID | Qui | Tâche | h | Fini quand |
|---|---|---|---|---|---|
| ☐ | T021 | Nicolas | En-tête de paquet, séquences et acks | 6 | Header (protocole, seq, ack, ack_bits) + comparaison modulaire, tests unitaires. |
| ☐ | T059 | Coéquipier | Survivant 2 (Ilian) : modèle et rig | 5 | Corps + bras FPS riggés, animations de base réutilisées. |

### Semaine 7 — du 28 déc. 2026 · M1 · Nicolas 6 h · Coéquipier 4 h

| | ID | Qui | Tâche | h | Fini quand |
|---|---|---|---|---|---|
| ☐ | T022 | Nicolas | Canal fiable et retransmission | 6 | Messages fiables livrés malgré 20 % de perte simulée en test. |
| ☐ | T060 | Coéquipier | Prototype d'ambiance | 4 | Scène de test avec éclairage, brouillard, végétation au vent et cycle jour/nuit simple. |

### Semaine 8 — du 4 janv. 2027 · M1 · Nicolas 12 h · Coéquipier 5 h

| | ID | Qui | Tâche | h | Fini quand |
|---|---|---|---|---|---|
| ☐ | T023 | Nicolas | Handshake avec challenge et timeouts | 6 | Connexion, refus de version, timeout 10 s, déconnexion explicite. |
| ☐ | T024 | Nicolas | Messages du protocole (inputs, état) | 4 | Types de messages sérialisés et testés. |
| ☐ | T025 | Nicolas | Simulateur de latence et de perte | 2 | Latence, gigue, perte et duplication configurables. |
| ☐ | T061 | Coéquipier | Survivant 3 (Teo) : modèle et rig | 5 | Corps + bras FPS riggés. |

### Semaine 9 — du 11 janv. 2027 · M1 · Nicolas 12 h · Coéquipier 5 h

| | ID | Qui | Tâche | h | Fini quand |
|---|---|---|---|---|---|
| ☐ | T026 | Nicolas | Sessions et diffusion de l'état | 5 | Le serveur gère jusqu'à 4 joueurs et diffuse leurs positions à chaque tick. |
| ☐ | T027 | Nicolas | Client Unreal connecté au serveur | 5 | Connexion via OdysseeNet, affichage des autres joueurs. |
| ☐ | T028 | Nicolas | Déploiement du serveur sur le homelab | 2 | Conteneur LXC, service systemd, odyssee.lan, Uptime Kuma. |
| ☐ | T062 | Coéquipier | Maquettes d'UI (Figma) | 5 | HUD, inventaire, artisanat, journal d'indices, menu principal. |

### Semaine 10 — du 18 janv. 2027 · M2 · Nicolas 12 h · Coéquipier 5 h

| | ID | Qui | Tâche | h | Fini quand |
|---|---|---|---|---|---|
| ☐ | T029 | Nicolas | Simulation de mouvement partagée | 5 | shared/sim : marche, course (endurance), saut + collisions de base, utilisé par client et serveur. |
| ☐ | T030 | Nicolas | Tests de déterminisme | 3 | Même suite d'inputs = même état, client et serveur. |
| ☐ | T031 | Nicolas | Prédiction côté client | 4 | Le joueur local bouge sans attendre le serveur. |
| ☐ | T063 | Coéquipier | Survivant 4 (Nadia) : modèle et rig | 5 | Corps + bras FPS riggés. |

### Semaine 11 — du 25 janv. 2027 · M2 · Nicolas 12 h · Coéquipier 6 h

| | ID | Qui | Tâche | h | Fini quand |
|---|---|---|---|---|---|
| ☐ | T032 | Nicolas | Réconciliation | 5 | Correction + rejeu des inputs, lissage des petites erreurs. |
| ☐ | T033 | Nicolas | Interpolation des entités distantes | 5 | Les autres joueurs bougent sans saccades à 150 ms + 5 % de perte. |
| ☐ | T034 | Nicolas | HUD de debug réseau | 2 | RTT, perte, ko/s, corrections affichés à l'écran. |
| ☐ | T064 | Coéquipier | Kit de végétation low-poly | 6 | Arbres, buissons, herbe, lierre, mousse ; herbe animée par le vent dans Unreal. |

### Semaine 12 — du 1 févr. 2027 · M2 · Nicolas 4 h · Coéquipier 4 h

| | ID | Qui | Tâche | h | Fini quand |
|---|---|---|---|---|---|
| ☐ | T035 | Les deux | Semaine tampon + post d'avancement | 4 | Retard rattrapé, post public M1/M2 publié. |

### Semaine 13 — du 8 févr. 2027 · M3 · Nicolas 0 h · Coéquipier 12 h

| | ID | Qui | Tâche | h | Fini quand |
|---|---|---|---|---|---|
| ☐ | T036 | Coéquipier | Terrain de la zone de départ | 6 | Relief, chemins et lieux de l'Abri (docs/ZONES.md) dans Unreal. |
| ☐ | T037 | Coéquipier | Kit de ruines modulaires | 6 | Murs, sols, débris, véhicules, végétation de ruines exportés vers Unreal. |

### Semaine 14 — du 15 févr. 2027 · M3 · Nicolas 12 h · Coéquipier 4 h

| | ID | Qui | Tâche | h | Fini quand |
|---|---|---|---|---|---|
| ☐ | T038 | Les deux | Acteurs marqueurs de gameplay | 4 | Nicolas crée les acteurs ResourceSpawn, ClueSpot, BossArena, ZoneGate ; le coéquipier les place dans la zone de départ. |
| ☐ | T039 | Nicolas | Outil d'export de la carte | 6 | Un utilitaire d'éditeur Unreal produit le fichier monde serveur (heightmap, collisions, marqueurs). |
| ☐ | T040 | Nicolas | Format du fichier monde | 2 | Format documenté et versionné. |

### Semaine 15 — du 22 févr. 2027 · M3, M4 · Nicolas 12 h · Coéquipier 12 h

| | ID | Qui | Tâche | h | Fini quand |
|---|---|---|---|---|---|
| ☐ | T041 | Nicolas | Chargement de la carte et collisions serveur | 8 | Le serveur charge la carte par chunks et applique les collisions du terrain. |
| ☐ | T042 | Nicolas | Zone d'intérêt (AOI) | 4 | Chaque joueur ne reçoit que les entités proches. |
| ☐ | T065 | Coéquipier | Habillage de la zone de départ | 6 | Centre de stase, village, ferme, pont effondré, premier cocon ; occlusion culling en place. |
| ☐ | T071 | Coéquipier | Postes d'artisanat (Blender) | 6 | Établi, forge et table de cartographie : modèles posables, .fbx et icônes. |

### Semaine 16 — du 1 mars 2027 · M4 · Nicolas 16 h · Coéquipier 11 h

| | ID | Qui | Tâche | h | Fini quand |
|---|---|---|---|---|---|
| ☐ | T043 | Nicolas | ECS serveur | 6 | Entités et composants, système de mise à jour par tick. |
| ☐ | T044 | Nicolas | Ressources récoltables avec repousse | 6 | Arbres et rochers récoltables, repousse côté serveur. |
| ☐ | T066 | Coéquipier | Objets, outils et ressources | 5 | Hache, pioche, ressources au sol, icônes d'inventaire. |
| ☐ | T068 | Les deux | Matériaux transformés et munitions | 4 | Planches, bâton, lingot de métal, munitions de pistolet et de fusil : .blend, .fbx et icônes d'inventaire, dans le style des outils. |
| ☐ | T070 | Coéquipier | Objet carte (Blender) | 2 | Carte vierge (seule la zone de départ dessinée) : .blend, .fbx et icône d'inventaire. |

### Semaine 17 — du 8 mars 2027 · M4 · Nicolas 12 h · Coéquipier 8 h

| | ID | Qui | Tâche | h | Fini quand |
|---|---|---|---|---|---|
| ☐ | T045 | Nicolas | Inventaire synchronisé | 6 | Inventaire serveur, mis à jour via le canal fiable. |
| ☐ | T046 | Nicolas | Objets et recettes data-driven | 6 | Objets et recettes décrits dans des fichiers de données. |
| ☐ | T072 | Coéquipier | Animations de combat | 8 | Animations de docs/GAMEPLAY.md § 7 (bras FPS + corps entier), intégrées dans Unreal. |

### Semaine 18 — du 15 mars 2027 · M4 · Nicolas 10 h · Coéquipier 8 h

| | ID | Qui | Tâche | h | Fini quand |
|---|---|---|---|---|---|
| ☐ | T047 | Nicolas | Artisanat | 4 | Fabriquer à la main, à l'établi ou à la forge (postes posés dans le monde) consomme les ressources, validé par le serveur. |
| ☐ | T048 | Coéquipier | UI d'inventaire et d'artisanat | 8 | Maquettes Figma intégrées en UMG, glisser-déposer, branchées sur l'état fourni par OdysseeNet. |
| ☐ | T074 | Nicolas | Exploration et carte commune (serveur) | 6 | Chunks vus par joueur, report à une table de cartographie, carte commune sauvegardée avec le monde. |

### Semaine 19 — du 22 mars 2027 · M4 · Nicolas 12 h · Coéquipier 12 h

| | ID | Qui | Tâche | h | Fini quand |
|---|---|---|---|---|---|
| ☐ | T049 | Les deux | Indices lisibles et journal du joueur | 8 | Lire un journal ou une inscription l'ajoute au journal (commun + personnel). |
| ☐ | T050 | Les deux | Tampon + post d'avancement M3/M4 | 4 | Retard rattrapé, post publié. |

### Semaine 20 — du 29 mars 2027 · M5 · Nicolas 16 h · Coéquipier 10 h

| | ID | Qui | Tâche | h | Fini quand |
|---|---|---|---|---|---|
| ☐ | T051 | Les deux | Santé, faim, soif, endurance | 6 | Stats simulées par le serveur (Nicolas), HUD côté client (coéquipier). |
| ☐ | T052 | Nicolas | Mort, sac au sol et réapparition | 6 | Inventaire laissé sur place, récupérable ; réapparition au point de repos. |
| ☐ | T076 | Les deux | GPS et piles | 4 | Flèche vers le prochain point d'intérêt, détecteur de cocons, piles qui s'usent (lampe et GPS). |

### Semaine 21 — du 5 avr. 2027 · M4, M5 · Nicolas 12 h · Coéquipier 9 h

| | ID | Qui | Tâche | h | Fini quand |
|---|---|---|---|---|---|
| ☐ | T053 | Nicolas | Persistance SQLite | 10 | Joueurs, inventaires et modifications du monde sauvegardés et rechargés. |
| ☐ | T054 | Nicolas | Test de redémarrage du serveur | 2 | Arrêt/redémarrage sans perte, automatisé. |
| ☐ | T073 | Coéquipier | Notifications du HUD | 3 | Messages courts de docs/GAMEPLAY.md § 6 : 3 au maximum, regroupés, sans interrompre le jeu. |
| ☐ | T075 | Coéquipier | Grande carte plein écran | 6 | Touche M : biomes dévoilés, chemins, points d'intérêt, position des joueurs ; pas de mini-carte. |

### Semaine 22 — du 12 avr. 2027 · M5 · Nicolas 18 h · Coéquipier 14 h

| | ID | Qui | Tâche | h | Fini quand |
|---|---|---|---|---|---|
| ☐ | T055 | Les deux | Menu principal et serveur local intégré | 8 | Solo (serveur en arrière-plan), Rejoindre, Favoris, Héberger ; choix du survivant. |
| ☐ | T056 | Nicolas | Codes d'invitation V1 | 4 | Connexion refusée sans le bon code. |
| ☐ | T077 | Les deux | Menu pause, touches et sauvegardes | 6 | Pause réelle en solo, touches modifiables, sauvegarder, charger, nouvelle partie ; sauvegarde automatique. |

### Semaine 23 — du 19 avr. 2027 · M5 · Nicolas 12 h · Coéquipier 12 h

| | ID | Qui | Tâche | h | Fini quand |
|---|---|---|---|---|---|
| ☐ | T057 | Les deux | Playtest, corrections et sortie du MVP | 12 | Session de jeu avec des amis sur le homelab, bugs bloquants corrigés, post MVP. |
