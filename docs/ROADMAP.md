# Roadmap & jalons

Le détail des tâches, semaine par semaine, est dans [PLANNING.md](PLANNING.md).

Chaque jalon se termine par quelque chose de **jouable ou démontrable**, et fait l'objet d'un point d'avancement public.

## M0 — Fondations ⏳
- [x] Dépôt, structure CMake (shared / server / client / tests)
- [x] Documentation de conception (vision, architecture, réseau)
- [x] Sérialisation binaire testée (`ByteWriter` / `ByteReader`)
- [x] Boucle serveur à pas fixe
- [x] Fenêtre client 3D (raylib)
- [ ] CI GitHub Actions (build + tests)
- [ ] Logger commun

**Démo :** le serveur tourne à 20 ticks/s, le client affiche une scène 3D.

## M0.5 — Spike Godot (validation de l'ADR 0002)
- [ ] GDExtension qui embarque `odyssee_shared`, compilée sous Linux et Windows
- [ ] Personnage `.glb` animé (premier modèle Blender) dans une scène Godot
- [ ] Export Windows fonctionnel
- [ ] Décision : ADR 0002 → accepté (on migre le client) ou rejeté (on garde raylib)

## M1 — Premier contact réseau
- [ ] Couche UDP (sockets non bloquantes, abstraction POSIX/Win32)
- [ ] En-tête de paquet, séquences, acks, canal fiable
- [ ] Handshake avec challenge, timeouts, déconnexion
- [ ] Plusieurs clients voient leurs cubes respectifs bouger (sans prédiction)
- [ ] Simulateur de latence/perte
- [ ] Sockets Linux **et** Windows (Winsock)
- [ ] Premier déploiement du serveur sur le homelab (LXC)

**Démo :** 2–3 clients connectés au serveur du homelab, chacun voit les autres se déplacer.

## M2 — Mouvement fluide
- [ ] Simulation de mouvement partagée (`shared/sim`)
- [ ] Prédiction client + réconciliation
- [ ] Interpolation des entités distantes
- [ ] HUD de debug réseau (RTT, perte, corrections)

**Démo :** déplacement fluide même avec 150 ms de latence simulée et 5 % de perte.

## M3 — Un monde à explorer
- [ ] Zone de départ construite à la main dans Godot (terrain + premiers décors Blender)
- [ ] Marqueurs de gameplay + outil d'export de la carte pour le serveur
- [ ] Chargement de la carte par chunks côté serveur
- [ ] Collisions avec le terrain, caméra 3ᵉ personne
- [ ] Zone d'intérêt côté serveur

**Démo :** explorer la zone de départ à plusieurs.

## M4 — Récolter & fabriquer
- [ ] ECS serveur
- [ ] Ressources récoltables avec repousse
- [ ] Inventaire synchronisé (canal fiable)
- [ ] Artisanat et recettes data-driven
- [ ] UI d'inventaire / artisanat
- [ ] Premiers indices lisibles + journal du joueur

**Démo :** couper un arbre, fabriquer une hache, couper plus vite.

## M5 — Survivre & persister
- [ ] Santé, faim, soif, endurance
- [ ] Persistance SQLite (joueurs, chunks modifiés, inventaires)
- [ ] Redémarrage du serveur sans perte
- [ ] Menu principal : Solo (serveur local intégré), Multijoueur (rejoindre, favoris, héberger)
- [ ] Codes d'invitation (V1)

**Démo :** déconnexion/reconnexion le lendemain, tout est resté en place ; une partie solo peut être ouverte en coop. → **MVP jouable**

## Au-delà du MVP (v1)
Artefacts et zones verrouillées, boss, construction, faune et combat, cycle jour/nuit, chat & groupes, annuaire de codes d'invitation (V2), hébergement derrière NAT, tests de charge à 50 bots, déploiement d'un serveur public.
