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
Chaque paquet acquitte donc implicitement les précédents ; un message fiable non acquitté après **100 ms** est renvoyé
(délai fixe pour l'instant, `ReliableSender` ; l'adapter à l'aller-retour mesuré reste à faire).

## Format d'un paquet

Tous les entiers sont en **little-endian**. Taille max : **1200 octets** (sous la MTU usuelle).

```
┌──────────┬──────────┬──────────┬──────────────┬────────────┬──────────────────┐
│ protocol │ sequence │   ack    │  ack_bits    │ PacketKind │ contenu…         │
│  u16     │  u16     │  u16     │  u32         │  u8        │ selon le type    │
└──────────┴──────────┴──────────┴──────────────┴────────────┴──────────────────┘
```

L'en-tête fait 10 octets ; l'octet `PacketKind` qui le suit dit comment lire le contenu : message de
connexion (voir « Connexion ») ou paquet de jeu (voir « Paquets de jeu »).

Les numéros de séquence sur 16 bits rebouclent ; la comparaison se fait en arithmétique modulaire.

## Connexion

```
Client                                        Serveur
  │── ConnectRequest(sel, pseudo, rembourrage) ──▶│  version (en-tête), pseudo, place libre ?
  │◀────────── Challenge(sel, jeton u64) ─────────│  jeton calculé, rien n'est retenu
  │── ChallengeResponse(sel, jeton, pseudo) ─────▶│  jeton recalculé : le client reçoit bien à cette adresse
  │◀──────── Accepted(playerId, jeton) ───────────│  numéro de joueur 0 à 3 ; le jeton devient le jeton de session
  │◀═══ paquets de jeu, KeepAlive, Disconnect ═══▶│  tous portent le jeton de session
```

Format d'un paquet de connexion : `PacketHeader | PacketKind (u8) | contenu`. Les numéros de séquence
de l'en-tête ne servent qu'après la connexion (valeurs à 0 pendant la poignée de main).

| Type (`PacketKind`) | Sens | Contenu |
|---|---|---|
| `ConnectRequest` = 1 | client → serveur | sel (u64), pseudo (chaîne, 16 octets max), puis des zéros jusqu'à **64 octets** de contenu au moins |
| `Challenge` = 2 | serveur → client | sel (u64, celui de la demande), jeton (u64) |
| `ChallengeResponse` = 3 | client → serveur | sel (u64), jeton (u64), pseudo (chaîne) |
| `Accepted` = 4 | serveur → client | numéro de joueur (u8), jeton de session (u64) |
| `Refused` = 5 | serveur → client | raison (u8) : 1 mauvaise version, 2 serveur plein, 3 pseudo invalide, 4 trop de joueurs depuis cette adresse IP |
| `Disconnect` = 6 | les deux | jeton de session (u64) — envoyé 3 fois par celui qui part (client qui quitte, serveur qui s'arrête) : il peut se perdre |
| `KeepAlive` = 7 | les deux | jeton de session (u64) — chaque seconde sans autre trafic |

Les valeurs de `PacketKind` et de `Refused` sont **figées** : un client d'une autre version doit pouvoir
lire un refus pour afficher « mauvaise version ».

### Défi sans état, jeton imprévisible

Le serveur **ne retient rien** avant `ChallengeResponse`. Le jeton du défi est un code d'authentification
(`server/ChallengeTokens`) :

```
jeton = SipHash-2-4(secret du serveur, IP | port | sel | période | pseudo)
```

- **Secret** : 128 bits tirés de `std::random_device` au démarrage du serveur (`SipKey::random()`), jamais
  envoyés. Sans lui, observer des jetons n'aide pas à prédire les suivants (ce que permettait la sortie
  brute d'un `mt19937_64`). Dans les tests, le secret est dérivé d'une graine : tout est reproductible.
- **Sel** : nombre aléatoire choisi par le client à chaque tentative. Le défi le répète (le client ignore
  un défi qu'il n'a pas demandé) et il rend le jeton unique à cette tentative.
- **Période** : numéro de la tranche de 5 s en cours (`kHandshakeTimeout`). Le serveur accepte le jeton
  de la période en cours ou de la précédente : un défi vaut donc de 5 à 10 s.
- `ChallengeResponse` répète le sel et le pseudo : le serveur recalcule le jeton et compare. Un jeton ne
  vaut que pour l'adresse, le sel et le pseudo de la demande.
- Il n'y a plus de table de poignées de main à saturer : un flot de fausses demandes ne consomme pas de
  mémoire et ne bloque pas les vrais joueurs.

### Jeton de session

Le jeton du défi accepté devient le **jeton de session**, connu des deux côtés (`Accepted` le répète :
le client ignore une acceptation forgée). Il est ensuite écrit dans **chaque** paquet d'après-connexion,
dans les deux sens : paquets de jeu, `KeepAlive`, `Disconnect`.

- Côté serveur, il est vérifié **avant tout effet** (`ConnectionService::authenticatedLink`) : un paquet
  venant de l'adresse d'un joueur mais sans son jeton n'est ni donné à sa session, ni compté comme signe
  de vie, ni décompté de son débit ; il ne peut pas le déconnecter.
- Côté client (`ClientConnection`, `Session`), un `Disconnect`, un `KeepAlive` ou un paquet de jeu sans le
  jeton est ignoré : usurper l'adresse du serveur ne ferme pas la connexion.
- Le jeton protège d'un tiers qui **devine** une adresse, pas de quelqu'un qui voit passer les paquets :
  rien n'est chiffré.

### Pertes, délais, reconnexion

- **Pertes** : le client renvoie sa demande (puis sa réponse) toutes les 250 ms ; le serveur répond la
  même chose à une demande en double (même défi) et à une réponse en double (même acceptation : cas de
  l'`Accepted` perdu).
- **Délais** : poignée de main abandonnée après 5 s côté client ; joueur connecté retiré après 10 s
  sans paquet authentifié.
- **Reconnexion depuis la même adresse** (client redémarré sur le même port) : un `ConnectRequest` venant
  de l'adresse d'un joueur connecté ne vaut qu'un nouveau défi et ne touche à rien — sinon n'importe qui
  pourrait déconnecter un joueur en usurpant son adresse. C'est la **réponse au défi**, avec un nouveau
  jeton valide, qui remplace l'ancienne connexion : session neuve, joueur réapparu, nouvelle
  **génération** de connexion.
- **Réponse rejouée** : le serveur garde les jetons des 64 dernières sessions terminées ; une vieille
  `ChallengeResponse` (doublon en retard ou rejeu) ne recrée pas de joueur fantôme.
- **Arrêt du serveur** : il envoie `Disconnect` (3 fois) à tous les joueurs.

### Protections contre les abus

- **Anti-amplification** : une réponse de poignée de main n'est jamais plus grosse que le datagramme qui
  l'a provoquée. `ConnectRequest` est donc rembourré (75 octets, pour un `Challenge` de 27) et une demande
  plus courte est ignorée. Le serveur ne peut pas servir à amplifier un envoi vers une adresse usurpée.
- **Plafond par adresse IP** : au plus `maxPlayersPerIp` joueurs connectés depuis une même IP
  (`ClientRegistry::Limits`). Par défaut 4, le nombre de places : quatre amis derrière la même box
  partagent une IP ; à baisser pour un serveur public.
- **Débit** (`server/TokenBucket`, `server/IpRateLimiter`, réglages dans `ConnectionService::Limits`) :
  - au plus **1024 datagrammes traités par tick** ; le reste attend dans le tampon du système ;
  - paquets sans jeton de session valide (poignées de main, bruit) : un seau à jetons **par adresse IP**,
    32 en rafale puis 16 par seconde ; la table des adresses suivies est bornée (4096) ;
  - paquets d'un joueur connecté (jeton vérifié) : un seau **par joueur**, 120 par seconde.
- **Journal** : les paquets écartés (hors débit, invalides, trop gros) sont comptés et résumés au plus
  toutes les 10 s, jamais journalisés un par un.

Code : `shared/net/ClientConnection` (client), `server/ClientRegistry` et `server/ConnectionService`.

## Paquets de jeu (T024)

Une fois connecté, tout passe par des paquets `Game` = 8 (`PacketKind`), dans les deux sens.
Les champs `sequence` / `ack` / `ackBits` de l'en-tête y servent : ils acquittent les paquets, et donc
les messages fiables qu'ils transportaient (renvoyés sinon, voir `ReliableSender`).

```
PacketHeader | Game (u8) | jeton de session (u64)
                         | fiables : count (u8) | [id (u16) | size (u16) | data] × count
                         | non fiables : count (u8) | [type (u8) | size (u16) | data] × count
```

Un paquet sans message (deux compteurs à 0) sert juste à acquitter. Au plus 64 messages fiables et
32 non fiables par paquet ; le tout tient dans 1200 octets. Un paquet dont le jeton n'est pas celui de
la session est ignoré avant d'être analysé ; un numéro de séquence plus de 1024 au-delà du dernier reçu
aussi (un seul paquet ne doit pas pouvoir décaler la fenêtre d'acquittement d'un demi-tour).

Canal fiable : l'émetteur n'envoie que les messages à moins de 512 numéros du plus ancien non acquitté
(le récepteur en accepte 1024 d'avance) et refuse d'en empiler plus de 4096 en attente.

### Messages non fiables (`GameMessageType`, valeurs figées)

| Type | Sens | Contenu |
|---|---|---|
| `Inputs` = 1 | client → serveur | `firstTick` (u32, ≥ 1), `count` (u8, 1 à 16), puis `count` inputs de ticks consécutifs, du plus ancien au plus récent |
| `Snapshot` = 2 | serveur → client | `serverTick` (u32), `lastProcessedInput` (u32), `self` (8 octets), `count` (u8, ≤ 60), puis `count` entités |

- **Input** (8 octets) : `moveForward` (i8), `moveRight` (i8), `yaw` (u16), `pitch` (i16), `buttons` (u16).
  Boutons : `Jump` 1, `Sprint` 2, `Crouch` 4, `Use` 8, `Primary` 16, `Secondary` 32 ; tout autre bit est refusé.
- **Entité** (18 octets) : `id` (u16), `kind` (u8, `Player` = 1), `x`, `y`, `z` (i32), `yaw` (u16), `flags` (u8 :
  au sol 1, accroupi 2, court 4, épuisé 8).
- **`self`** : vitesse `vx`, `vy`, `vz` (i16, 1/256 m par tick) et endurance (u16) du joueur destinataire. Avec sa
  propre entité, c'est son état de simulation complet : sa prédiction repart exactement de là (T029).
- `lastProcessedInput` : dernier tick d'input **de ce joueur** appliqué par le serveur (0 tant qu'aucun),
  base de la réconciliation. Les ticks d'input sont ceux du client et **commencent à 1** (un lot qui
  commence à 0 est refusé). Le serveur n'accepte que des `Inputs` : un `Snapshot` reçu d'un client est
  ignoré sans être décodé.
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

Version du protocole : **3** — poignée de main sans état (sel, pseudo répété, demande rembourrée),
jeton de session dans `Accepted`, `Disconnect`, `KeepAlive` et les paquets de jeu, ticks d'input à partir
de 1. (2 : ajout des paquets de jeu.)

## Sessions et diffusion de l'état (T026)

- **`shared/net/Session`** : l'échange de paquets de jeu avec l'autre côté, sans réseau (comme
  `ClientConnection`) ; le serveur en a une par joueur, le client une seule. Elle numérote les paquets,
  acquitte ceux reçus (un paquet doublon, trop vieux ou invalide est ignoré **avant** d'être compté
  comme reçu), renvoie les messages fiables non acquittés, abandonne les non fiables qui ne tiennent
  plus dans le paquet, et mesure l'aller-retour (moyenne glissante à 1/8, comme TCP).
- **Côté serveur, à chaque tick** (`server/GameStep`) : lire les paquets → donner les inputs au monde →
  simuler **chaque joueur d'un pas** (au plus un input appliqué) → construire un snapshot par joueur →
  lui envoyer un paquet. Un paquet de jeu ne crée jamais de connexion : il n'est accepté que de
  l'adresse d'un joueur connecté, avec son jeton de session.
- **Génération de connexion** : chaque connexion acceptée reçoit un numéro jamais réutilisé
  (`server/ConnectedPlayer`). Le monde le compare à chaque tick : si une place a changé de génération
  (joueur parti et place reprise dans le même tick, ou reconnexion), le joueur **réapparaît** au lieu
  d'hériter de la position, des inputs en attente et du dernier tick appliqué du précédent.
- **Inputs** (`server/InputQueue`) : on ne garde que les inputs plus récents que le dernier reçu (la
  redondance des lots crée des doublons) ; un trou est sauté ; au-delà de 8 inputs en attente (400 ms),
  les plus anciens sont sautés pour que le joueur ne soit pas joué de plus en plus en retard. Un tick
  plus d'une minute (1200 ticks) au-delà du dernier reçu est refusé : le client ne doit pas faire
  avancer ses ticks d'input sans les envoyer.
- **Tick sans input** (`GameWorld::step`) : un joueur est simulé à chaque tick du serveur, qu'un input
  soit arrivé ou non — sinon un client qui se tait resterait suspendu en l'air, hors gravité.
  - Si le joueur a déjà envoyé des inputs, le tick manquant est **deviné** : le serveur rejoue le
    dernier input et le compte comme traité (`lastProcessedInput` avance d'un tick). Si le vrai input
    arrive ensuite, il est ignoré. Le serveur reste ainsi aligné sur les ticks du client, sans
    accumuler de retard.
  - Au-delà de **4 ticks devinés d'affilée** (200 ms), ou avant le tout premier input, le serveur simule
    un input **neutre** (aucune touche, même orientation, toujours accroupi si le joueur l'était) et
    n'avance plus `lastProcessedInput` : le joueur retombe, s'arrête, et son client peut reprendre où
    il en était.
  - **Réconciliation** : rien ne change côté client. Le snapshot donne l'état complet après le tick
    `lastProcessedInput` ; le client repart de cet état et rejoue ses inputs suivants. Si l'input
    deviné était le bon (cas courant : les inputs changent peu d'un tick à l'autre), sa prédiction est
    exacte ; sinon, ou après des ticks neutres, il constate un écart et se corrige comme pour toute
    autre correction du serveur.
- **Snapshot** : tous les joueurs (pas encore de zone d'intérêt), et le dernier input appliqué du joueur
  destinataire. Le client garde le plus récent (`serverTick`) : un snapshot arrivé en retard est ignoré.
- **Monde** (`server/GameWorld`) : les joueurs apparaissent côte à côte, à 2 m d'écart, au point
  d'apparition de la carte (salle des capsules du centre de stase), et sont déplacés par la simulation
  partagée (`sim::step`, T029) sur le sol et les murs de la carte (`world::WorldTerrain`, T041 :
  `--world` côté serveur **et** client, même fichier). Sans carte : sol plat provisoire.
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

- Lecture défensive : chaque champ est vérifié (`ByteReader` renvoie `std::optional`), toute longueur est bornée ;
  les décodeurs sont testés sur des octets aléatoires (`tests/test_fuzz_decoders.cpp`).
- Poignée de main sans état, jetons imprévisibles, jeton de session, anti-amplification, plafond par IP et
  limitation de débit : voir « Connexion ».
- Bornes : saut de numéro de paquet (1024), saut de tick d'input (une minute), fenêtre et file du canal
  fiable, positions de la simulation (± 2^30 unités), rattrapage de ticks du serveur (5).
- Le client n'est jamais cru : positions, inventaires et dégâts sont calculés uniquement par le serveur, et
  un joueur est simulé à chaque tick même s'il n'envoie rien.
- Pas encore fait : chiffrement et authentification des paquets (un tiers qui voit passer le trafic peut
  lire le jeton de session), déconnexion automatique sur abus répétés.

## Outils de test

- Simulateur de conditions réseau (`shared/net/NetworkSimulator`, T025) : latence, gigue (qui réordonne),
  perte et duplication des datagrammes **envoyés** ; pour dégrader les deux sens, l'activer des deux côtés.
  Serveur : `odyssee_server --lag 120 --jitter 30 --loss 5 --dup 1` (ms et %). Tirages reproductibles
  (graine, générateur `mt19937_64` sans distribution de la bibliothèque standard). Le serveur se réveille
  entre deux ticks pour envoyer les datagrammes retardés à l'heure ; l'arrêt du serveur ne passe pas
  par la simulation.
- Clients « bots » headless pour tester 4 joueurs dans les pires conditions (combats de boss, zones denses).
- Statistiques à l'écran : RTT, perte, ko/s entrants/sortants, corrections de prédiction.
