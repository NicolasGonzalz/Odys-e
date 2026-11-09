# Roadmap & jalons

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

## M1 — Premier contact réseau
- [ ] Couche UDP (sockets non bloquantes, abstraction POSIX/Win32)
- [ ] En-tête de paquet, séquences, acks, canal fiable
- [ ] Handshake avec challenge, timeouts, déconnexion
- [ ] Plusieurs clients voient leurs cubes respectifs bouger (sans prédiction)
- [ ] Simulateur de latence/perte

**Démo :** 2–3 clients connectés, chacun voit les autres se déplacer.

## M2 — Mouvement fluide
- [ ] Simulation de mouvement partagée (`shared/sim`)
- [ ] Prédiction client + réconciliation
- [ ] Interpolation des entités distantes
- [ ] HUD de debug réseau (RTT, perte, corrections)

**Démo :** déplacement fluide même avec 150 ms de latence simulée et 5 % de perte.

## M3 — Un monde à explorer
- [ ] Chunks + génération procédurale seedée (bruit, biomes)
- [ ] Streaming des chunks selon la position
- [ ] Collisions avec le terrain, caméra 3ᵉ personne
- [ ] Zone d'intérêt côté serveur

**Démo :** marcher sur un grand terrain procédural à plusieurs.

## M4 — Récolter & fabriquer
- [ ] ECS serveur
- [ ] Ressources récoltables avec repousse
- [ ] Inventaire synchronisé (canal fiable)
- [ ] Artisanat et recettes data-driven
- [ ] UI d'inventaire / artisanat

**Démo :** couper un arbre, fabriquer une hache, couper plus vite.

## M5 — Survivre & persister
- [ ] Santé, faim, soif, endurance
- [ ] Persistance SQLite (joueurs, chunks modifiés, inventaires)
- [ ] Redémarrage du serveur sans perte

**Démo :** déconnexion/reconnexion le lendemain, tout est resté en place. → **MVP jouable**

## Au-delà du MVP (v1)
Construction, faune et combat, cycle jour/nuit, chat & groupes, tests de charge à 50 bots, déploiement d'un serveur public.
