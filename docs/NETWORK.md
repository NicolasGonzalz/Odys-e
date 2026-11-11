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
  │◀═══════ paquets de jeu (T024, T026) ════│
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

## Paquets de jeu (T024)

Une fois connecté, tout passe par des paquets `Game` = 8 (`PacketKind`), dans les deux sens.
Les champs `sequence` / `ack` / `ackBits` de l'en-tête y servent : ils acquittent les paquets, et donc
les messages fiables qu'ils transportaient (renvoyés sinon, voir `ReliableSender`).

```
PacketHeader | Game (u8) | fiables : count (u8) | [id (u16) | size (u16) | data] × count
                         | non fiables : count (u8) | [type (u8) | size (u16) | data] × count
```

Un paquet sans message (deux compteurs à 0) sert juste à acquitter. Au plus 64 messages fiables et
32 non fiables par paquet ; le tout tient dans 1200 octets.

### Messages non fiables (`GameMessageType`, valeurs figées)

| Type | Sens | Contenu |
|---|---|---|
| `Inputs` = 1 | client → serveur | `firstTick` (u32), `count` (u8, 1 à 16), puis `count` inputs de ticks consécutifs, du plus ancien au plus récent |
| `Snapshot` = 2 | serveur → client | `serverTick` (u32), `lastProcessedInput` (u32), `count` (u8, ≤ 60), puis `count` entités |

- **Input** (8 octets) : `moveForward` (i8), `moveRight` (i8), `yaw` (u16), `pitch` (i16), `buttons` (u16).
  Boutons : `Jump` 1, `Sprint` 2, `Crouch` 4, `Use` 8, `Primary` 16, `Secondary` 32 ; tout autre bit est refusé.
- **Entité** (18 octets) : `id` (u16), `kind` (u8, `Player` = 1), `x`, `y`, `z` (i32), `yaw` (u16), `flags` (u8).
- `lastProcessedInput` : dernier tick d'input **de ce joueur** appliqué par le serveur (0 tant qu'aucun),
  base de la réconciliation. Les ticks d'input sont ceux du client.
- Les messages fiables (événements de jeu, inventaire, chat) arrivent avec le gameplay : leur contenu
  commencera lui aussi par un type.

### Quantification (`shared/net/Quantize`)

| Grandeur | Codage | Précision |
|---|---|---|
| Position (m) | i32, 1/256 m | ≈ 4 mm, ± 8 000 km |
| Cap (yaw, °) | u16, un tour = 65536 | ≈ 0,0055°, rebouclé |
| Inclinaison (pitch, °) | i16, ±90° = ±32767 | ≈ 0,003° |
| Axe de déplacement | i8, ±1 = ±127 | 1/127 |

Le client simule avec l'input **quantifié** (celui qu'il envoie) : le serveur rejoue exactement la même
entrée. NaN et valeurs hors bornes sont ramenés dans l'intervalle ; un octet hors intervalle reçu
(axe à -128, pitch à -32768) rend l'input invalide. Tout message invalide, tronqué ou suivi d'octets en
trop est ignoré entier.

Version du protocole : **2** depuis l'ajout des paquets de jeu.

## Sessions et diffusion de l'état (T026)

- **`shared/net/Session`** : l'échange de paquets de jeu avec l'autre côté, sans réseau (comme
  `ClientConnection`) ; le serveur en a une par joueur, le client une seule. Elle numérote les paquets,
  acquitte ceux reçus (un paquet doublon, trop vieux ou invalide est ignoré **avant** d'être compté
  comme reçu), renvoie les messages fiables non acquittés, abandonne les non fiables qui ne tiennent
  plus dans le paquet, et mesure l'aller-retour (moyenne glissante à 1/8, comme TCP).
- **Côté serveur, à chaque tick** (`server/GameStep`) : lire les paquets → donner les inputs au monde →
  appliquer **au plus un input par joueur** → construire un snapshot par joueur → lui envoyer un paquet.
  Un paquet de jeu ne crée jamais de connexion : il n'est accepté que de l'adresse d'un joueur connecté.
- **Inputs** (`server/InputQueue`) : on ne garde que les inputs plus récents que le dernier reçu (la
  redondance des lots crée des doublons) ; un trou est sauté ; au-delà de 8 inputs en attente (400 ms),
  les plus anciens sont sautés pour que le joueur ne soit pas joué de plus en plus en retard.
- **Snapshot** : tous les joueurs (pas encore de zone d'intérêt), et le dernier input appliqué du joueur
  destinataire. Le client garde le plus récent (`serverTick`) : un snapshot arrivé en retard est ignoré.
- **Monde provisoire** (`server/GameWorld`) : les joueurs apparaissent dans la salle des capsules du
  centre de stase, à 2 m d'écart ; seule leur orientation suit l'input. Le mouvement viendra de la
  simulation partagée (T029), le point d'apparition de la carte chargée par le serveur (T041).
- **KeepAlive** : inutiles tant que des paquets de jeu circulent (ils comptent comme trafic des deux
  côtés : `ClientRegistry::touch/noteSent`, `ClientConnection::onGamePacketReceived/Sent`).

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
