# Modèle réseau & protocole

## Objectifs

- **4 joueurs** par monde (un par survivant), latences de 20 à 150 ms. Un serveur dédié peut héberger plusieurs mondes.
- Le monde reste riche en entités (faune, ressources, objets) : c'est le nombre d'**entités**, pas de joueurs, qui dimensionne le réseau.
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
  │── ConnectRequest(pseudo) ──────────────▶│  version (en-tête), pseudo, place libre ?
  │◀─────────── Challenge(jeton u64) ───────│  jeton aléatoire envoyé à l'adresse annoncée
  │── ChallengeResponse(jeton) ────────────▶│  prouve que le client possède bien cette adresse
  │◀────────── Accepted(playerId) ──────────│  numéro de joueur 0 à 3
  │◀═════════ état du monde (T024+) ════════│
```

Format d'un paquet de connexion : `PacketHeader | PacketKind (u8) | contenu`. Les numéros de séquence
de l'en-tête ne servent qu'après la connexion (valeurs à 0 pendant la poignée de main).

| Type (`PacketKind`) | Sens | Contenu |
|---|---|---|
| `ConnectRequest` = 1 | client → serveur | pseudo (chaîne, 16 octets max) |
| `Challenge` = 2 | serveur → client | jeton (u64) |
| `ChallengeResponse` = 3 | client → serveur | jeton (u64) |
| `Accepted` = 4 | serveur → client | numéro de joueur (u8) |
| `Refused` = 5 | serveur → client | raison (u8) : 1 mauvaise version, 2 serveur plein, 3 pseudo invalide |
| `Disconnect` = 6 | les deux | — (envoyé 3 fois, il peut se perdre) |
| `KeepAlive` = 7 | les deux | — (chaque seconde sans autre trafic) |

Les valeurs de `PacketKind` et de `Refused` sont **figées** : un client d'une autre version doit pouvoir
lire un refus pour afficher « mauvaise version ».

- **Pertes** : le client renvoie sa demande toutes les 250 ms ; le serveur répond la même chose
  à une demande en double (même défi, même acceptation).
- **Délais** : poignée de main abandonnée après 5 s ; joueur connecté retiré après 10 s sans paquet.
- **Protection** : 32 poignées de main simultanées au maximum ; jetons tirés d'une graine imprévisible.
- **Arrêt du serveur** : il envoie `Disconnect` à tous les joueurs.

Code : `shared/net/ClientConnection` (client), `server/ClientRegistry` et `server/ConnectionService`.

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

- Simulateur de conditions réseau (`shared/net/NetworkSimulator`, T025) : latence, gigue (qui réordonne),
  perte et duplication des datagrammes **envoyés** ; pour dégrader les deux sens, l'activer des deux côtés.
  Serveur : `odyssee_server --lag 120 --jitter 30 --loss 5 --dup 1` (ms et %). Tirages reproductibles
  (graine, générateur `mt19937_64` sans distribution de la bibliothèque standard). Le serveur se réveille
  entre deux ticks pour envoyer les datagrammes retardés à l'heure ; l'arrêt du serveur ne passe pas
  par la simulation.
- Clients « bots » headless pour tester 4 joueurs dans les pires conditions (combats de boss, zones denses).
- Statistiques à l'écran : RTT, perte, ko/s entrants/sortants, corrections de prédiction.
