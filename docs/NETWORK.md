# Modèle réseau & protocole

## Objectifs

- Jusqu'à **~50 joueurs** par serveur, latences de 20 à 150 ms.
- Déplacements perçus comme **instantanés** côté joueur local.
- Bande passante descendante raisonnable : viser **< 30 ko/s par client** en situation normale.
- Robustesse : un client malveillant ou un paquet corrompu ne doit jamais faire tomber le serveur.

## Transport : UDP + fiabilité sélective

TCP impose l'ordre et la retransmission de *tout* : un paquet perdu bloque les suivants (head-of-line blocking),
ce qui est inacceptable pour des positions qui sont périmées 50 ms plus tard. On utilise donc UDP avec trois canaux :

| Canal | Garanties | Usage |
|---|---|---|
| **Non fiable** | aucune | snapshots d'état, inputs (redondants) |
| **Fiable ordonné** | livraison + ordre | chat, inventaire, artisanat, événements de jeu |
| **Fiable non ordonné** | livraison | données de chunks |

Mécanisme de fiabilité : numéro de séquence par paquet + champ `ack` + bitfield des 32 derniers paquets reçus.
Chaque paquet acquitte donc implicitement les précédents ; un message fiable non acquitté après ~RTT×1,5 est renvoyé.

## Format d'un paquet

Tous les entiers sont en **little-endian**. Taille max : **1200 octets** (sous la MTU usuelle).

```
┌──────────┬──────────┬──────────┬──────────────┬──────────────────────────┐
│ protocol │ sequence │   ack    │  ack_bits    │ messages…                │
│  u16     │  u16     │  u16     │  u32         │ [type u8][len u16][data] │
└──────────┴──────────┴──────────┴──────────────┴──────────────────────────┘
```

Les numéros de séquence sur 16 bits rebouclent ; la comparaison se fait en arithmétique modulaire.

## Connexion

```
Client                                  Serveur
  │── ConnectRequest(version, nom) ────────▶│  vérifie version, capacité
  │◀─────────── Challenge(token) ───────────│  (anti-spoofing d'IP)
  │── ChallengeResponse(token) ────────────▶│
  │◀─── Accepted(playerId, tick, seed) ─────│
  │◀═════════ snapshots / chunks ═══════════│
```

Déconnexion : message explicite, ou timeout après 10 s sans paquet. Keep-alive implicite via les inputs.

## Synchronisation de l'état

### Côté client → serveur : inputs
À chaque tick client, on envoie l'input du tick (touches, orientation caméra, action) **plus les N derniers**
non acquittés, pour encaisser les pertes sans retransmission. Le serveur applique les inputs dans l'ordre des ticks,
et rejette ceux qui sont incohérents (vitesse impossible, action hors de portée…).

### Côté serveur → client : snapshots
- À chaque tick (ou un tick sur deux), le serveur envoie l'état des entités **dans la zone d'intérêt** du joueur.
- **Compression delta** : l'état est encodé par rapport au dernier snapshot acquitté par ce client.
- **Quantification** : positions en virgule fixe relative au chunk, angles sur 16 bits.
- Chaque snapshot contient le **dernier tick d'input traité** pour ce joueur → base de la réconciliation.

### Prédiction & réconciliation (joueur local)
1. Le client applique immédiatement son input avec la même fonction de simulation que le serveur (`shared/sim`).
2. Il conserve un historique `(tick, input, état prédit)`.
3. À la réception d'un snapshot : il remplace son état par l'état serveur au tick confirmé, puis **rejoue** les inputs suivants.
4. Si l'écart est faible, il lisse la correction visuellement au lieu de téléporter.

### Interpolation (autres entités)
Les entités distantes sont affichées avec un retard de ~2 snapshots (≈100 ms) et interpolées entre deux états connus.
Cela masque la gigue réseau au prix d'un léger décalage, acceptable pour un jeu non compétitif.

### Actions sur le monde (récolte, construction)
Le client affiche un retour immédiat (animation, son), mais le **résultat** (ressource obtenue, bloc posé)
n'est appliqué qu'à la confirmation du serveur. En cas de refus, l'effet visuel est annulé.

## Zone d'intérêt (AOI)

Le monde est découpé en chunks. Un joueur est abonné aux chunks dans un rayon donné autour de lui ;
il ne reçoit les entités et les modifications que de ces chunks. C'est la clé pour passer à l'échelle en nombre de joueurs.

## Sécurité

- Lecture défensive : chaque champ est vérifié (`ByteReader` renvoie `std::optional`), toute longueur est bornée.
- Limitation de débit par client, taille maximale de message, déconnexion sur abus répétés.
- Le client n'est jamais cru : positions, inventaires et dégâts sont calculés uniquement par le serveur.

## Outils de test

- Simulateur de conditions réseau intégré (latence, gigue, perte, duplication) activable des deux côtés.
- Clients « bots » headless pour tester la charge à 50+ joueurs.
- Statistiques à l'écran : RTT, perte, ko/s entrants/sortants, corrections de prédiction.
