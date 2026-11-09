# Modes de jeu & hébergement

## Principe : il y a toujours un serveur

Même en solo, le jeu passe par un serveur. Le client ne contient **aucune** logique d'autorité : il se connecte
toujours à un serveur, qu'il soit local, sur le PC d'un ami ou sur le homelab. Conséquences :

- un seul chemin de code à maintenir et tester ;
- passer du solo à la coop, c'est simplement ouvrir la partie à d'autres joueurs ;
- le binaire `odyssee_server` est livré avec le jeu.

## Menu principal

```
ODYSSÉE
 ├─ Solo
 │    └─ Choisir / créer un monde local      → serveur lancé en arrière-plan, 127.0.0.1, fermé aux autres
 ├─ Multijoueur
 │    ├─ Rejoindre avec un code               → ex. ODY-7KQ2
 │    ├─ Serveurs favoris                     → ex. « Monde persistant (homelab) »
 │    └─ Héberger ma partie                   → ton monde local, ouvert aux amis (duo / coop)
 ├─ Options
 └─ Quitter
```

- **Solo** : le client démarre `odyssee_server` en processus enfant (port local, accès limité à la machine),
  s'y connecte, et l'arrête en quittant. Le monde est sauvegardé localement.
- **Duo / coop** : même chose que le solo, mais le serveur accepte d'autres joueurs (nombre max réglable).
  « Duo » et « coop » ne sont pas deux modes techniques différents : seule la limite de joueurs change.
- **Monde persistant** : un serveur dédié qui tourne en permanence sur le homelab, même quand personne ne joue.

Un monde solo peut être ouvert plus tard en coop : c'est le même format de sauvegarde.

## Serveur dédié sur le homelab (Proxmox)

- **Conteneur LXC** Debian dédié, IP fixe, nom `odyssee.lan`, service systemd `odyssee-server`.
- Port **UDP 27015**.
- Surveillance dans Uptime Kuma, carte sur Homepage, sauvegardes de la base du monde (SQLite) dans le plan de sauvegarde existant.
- Mises à jour : déploiement du binaire Linux compilé par la CI.

## Se connecter

| Situation | Méthode |
|---|---|
| À la maison | `odyssee.lan:27015` directement |
| PC portable en déplacement | via le VPN du homelab : rien n'est exposé sur Internet |
| Amis (début) | ils rejoignent un VPN maillé (Tailscale/WireGuard) |
| Amis (plus tard) | redirection du port UDP sur la box + **code d'invitation** obligatoire |

### Codes d'invitation

1. **V1** : le code est un secret vérifié par le serveur pendant la connexion (étape de challenge, voir [NETWORK.md](NETWORK.md)).
   L'adresse du serveur est saisie ou enregistrée dans les favoris.
2. **V2** : un petit service « annuaire » traduit un code court (`ODY-7KQ2`) en adresse et port.
   Le joueur ne tape plus d'IP.
3. **Héberger depuis son PC** derrière une box (NAT) est le cas le plus difficile (traversée de NAT, relais) :
   on le traite après le reste. En attendant, la coop entre amis passe par le serveur du homelab.

## Plateformes

| Composant | Linux | Windows |
|---|---|---|
| Client (Godot) | ✅ | ✅ |
| Serveur intégré (solo / héberger) | ✅ | ✅ (sockets Winsock dès M1) |
| Serveur dédié (homelab) | ✅ | non prévu |
