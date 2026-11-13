# Systèmes de jeu en détail

> **Statut : brouillon (T069).** Complète [VISION.md](VISION.md) (la vue d'ensemble) : ici, comment chaque système
> fonctionne. Les chiffres (vitesses, durées, touches) sont **provisoires** et seront réglés en jouant.
> Voir aussi [MODES.md](MODES.md) (solo, coop, serveur) et [NETWORK.md](NETWORK.md).

Rappel technique : le serveur est **autoritaire**. Tout ce qui change le monde (fabriquer, explorer, sauvegarder)
est décidé par lui ; le client affiche et envoie des intentions. Les menus et la carte sont du client (Unreal),
leur contenu vient du serveur.

## 1. Déplacement : marche et course

| Action | Touche | Règle |
|---|---|---|
| Marcher | ZQSD / WASD | Par défaut. Ne coûte rien. ~3,5 m/s |
| Courir | Maj (maintenue) | ~6 m/s. Consomme de l'**endurance** ; impossible quand elle est vide |
| Sauter | Espace | Coûte un peu d'endurance |

- L'endurance remonte dès qu'on ne court plus (plus vite à l'arrêt qu'en marchant). Elle est partagée avec le
  combat (frapper, bloquer, esquiver) : courir avant un combat se paie.
- Le mouvement est calculé par **`shared/sim`**, à l'identique sur le client et le serveur (prédiction puis
  réconciliation). « Courir » est un simple drapeau dans l'input envoyé au serveur (T024).

### Besoins vitaux (T051)

- **Vie, faim, soif** : simulées par le serveur à chaque tick, montrées en % dans le HUD.
- La faim se vide en ~21 min et la soif en ~14 min de jeu ; **courir** les creuse deux fois plus vite.
- Faim ou soif à zéro : la vie baisse (~3 min pour tout perdre, deux fois plus vite si les deux sont vides).
- Faim et soif au-dessus de 60 % : la vie remonte lentement (~5 min pour la remplir).
- Chiffres provisoires (`server/src/Vitals.hpp`, `VitalsTuning`). Manger et boire : avec l'inventaire
  (T045) ; mort et réapparition : T052.

## 2. Artisanat : à la main, à l'établi, à la forge

Trois niveaux, chacun ouvre le suivant. Pour utiliser un poste, il faut être **à côté** (~3 m).

| Où | Ce qu'on y fait | Exemples |
|---|---|---|
| **À la main** (partout) | Le strict nécessaire pour survivre la première nuit | Bâton, hache et pioche de pierre, lance simple, torche, **établi** |
| **Établi** | Travailler le bois et assembler | Planches (depuis la bûche), arc, flèches, arbalète, coffres, **table de cartographie**, **forge** |
| **Forge** | Fondre et travailler le métal | Lingots (minerai + bois), hache et pioche de métal, pièces des armes à feu |

- Chaînes de matières : **bûche → planches** (établi), **minerai → lingot** (forge), **bâton + pierre → outils de
  pierre** (main). Les munitions d'armes à feu se **trouvent** dans les ruines (rares) ; en fabriquer reste à décider.
- Établi, forge et table de cartographie sont des **objets posés dans le monde** : ils restent (persistance) et,
  en coop, servent à tous.
- Recettes décrites dans des fichiers de données (T046), fabrication validée par le serveur (T047).

### Récolte (T044)

- **Arbres, rochers, buissons de fibres, filons de minerai** : types décrits dans `data/resources.txt`
  (objet gagné par coup, nombre de coups, cadence, outils, repousse ; format dans [DATA_FORMAT.md](DATA_FORMAT.md)).
- On récolte en **maintenant le clic gauche** (`Primary`) face à une ressource : à **2,5 m** au plus, dans un
  cône de **45°** de part et d'autre du regard (la hauteur du regard ne compte pas). Collé à elle (< 40 cm), le
  cap ne compte plus. Plusieurs à portée : la plus proche.
- **Cadence** : un coup toutes les `hit_ticks` avec un des outils indiqués (hache pour l'arbre, pioche pour
  le rocher), **deux fois plus lent à la main**. Certaines ressources (filon) ne se récoltent pas à la main.
- Après son dernier coup, la ressource est **épuisée** (affichée pâle) et **repousse** entière au bout de
  `regrow` ticks (5 min pour un arbre). Pas de repousse partielle : entamée, elle le reste.
- Le serveur décide de tout (portée, cadence, gain) ; le client ne fait que montrer. Le gain va dans
  l'**inventaire** (T045) ; plein, le reste est perdu. L'outil tenu (case active de la barre d'outils)
  viendra ensuite : en attendant, tout le monde récolte à la main.
- **Placement provisoire** : tant que la carte n'a pas de marqueurs de ressources (T038), le serveur les tire
  au hasard **avec une graine fixe** (toujours les mêmes) : ~160 entre 15 et 130 m du point d'apparition, sur
  le relief à découvert (ni trou, ni mur, ni toit, pente douce), 4 m d'écart au moins ; sans carte, une
  douzaine autour du point d'apparition.
### Inventaire (T045)

- 25 cases ; les 5 premières sont la **barre d'outils** (touches 1 à 5 du HUD). Piles limitées par objet
  (`stack` dans `data/items.txt`).
- Tenu par le serveur seul : récolte et artisanat y ajoutent, le joueur ne fait que réorganiser
  (déplacer une case : échange, ou regroupement d'une même pile).

### Outil tenu, artisanat, nourriture (T047)

- **Touches 1 à 5** : la case tenue de la barre d'outils part dans chaque input. Un **outil** tenu (hache,
  pioche) récolte à pleine vitesse ; sinon, mains nues (deux fois plus lent, filon impossible).
- **E** : mange ou boit le consommable tenu (`eat` / `drink` dans `data/items.txt`), une fois par appui.
  Pour l'instant : les **baies** (buissons de baies). Une source d'eau reste à concevoir.
- **Artisanat** : le joueur demande une recette ; le serveur vérifie poste et ingrédients, les prend au
  début, et ajoute le résultat après `ticks`. Une fabrication à la fois. Seules les recettes « à la main »
  marchent tant que les établis et forges ne sont pas posés dans le monde.

## 3. Carte et exploration

**Pas de mini-carte.** On se repère au paysage, et on consulte une **grande carte plein écran** quand on veut.

- **Ouvrir la carte** : touche **M**, ou utiliser l'objet **Carte** de l'inventaire. On la reçoit au réveil
  (carte vierge) ; comme les objets équipés, on la garde à la mort.
- **Au départ, seule la zone de départ (L'Abri) est dessinée.** Le reste est blanc.
- **On explore, puis on reporte** : en marchant, le joueur *mémorise* les régions qu'il traverse et les lieux qu'il
  voit. La carte **ne se met à jour qu'à une table de cartographie** (fabriquée à l'établi, posée dans sa base) :
  rentrer au camp devient un moment fort. Une notification rappelle qu'il y a du nouveau à reporter.
- **Les fragments de carte** : chaque biome a son fragment (plaine, ville, marais, montagne, forêt du Cœur), à
  trouver ou à gagner dans ce biome. Posé sur la table de cartographie, il dessine d'un coup les grandes lignes du
  biome (relief, eau, lieux principaux) ; l'exploration ajoute ensuite les détails. La grande carte s'assemble
  ainsi fragment par fragment, et les emplacements encore vides se voient sur la table.
- **Ce qui se dévoile** : le biome de chaque région (plaine, ville, marais, montagne…), les chemins, les points
  d'intérêt aperçus (ruines, villages, lieux nommés). Ce qu'on n'a pas vu reste blanc.
- **En coop, la carte est commune** : ce qu'un joueur reporte à la table apparaît pour tous. Chacun garde ses
  souvenirs non reportés jusqu'à son passage à une table.
- Sur la carte : sa position, celle des coéquipiers, et (v1) des marqueurs qu'on pose soi-même.
- Technique : le monde est découpé en chunks de 64 m. Le serveur retient, par joueur, les chunks vus et pas
  encore reportés, et la carte commune (sauvegardée avec le monde). Le client ne dessine que ce qu'on lui envoie.

## 4. GPS

Un vieil appareil retrouvé, qui fonctionne avec des **piles** (objet `pile`, à trouver dans les ruines).

- **Il pointe vers les nouveaux points d'intérêt** : une flèche indique la direction et la distance du prochain
  lieu découvert sur la carte mais pas encore visité. On peut en choisir un autre sur la grande carte.
- **Détecteur** : il bipe de plus en plus vite près d'un cocon ou d'un artefact.
- Les piles s'usent quand il est allumé ; sans pile, il s'éteint. La lampe torche utilise les mêmes piles.

## 5. Menus et sauvegarde

Menu principal : voir [MODES.md](MODES.md) (Solo, Multijoueur, Options, Quitter), avec en solo **Nouvelle partie**
et **Charger une partie**.

**Menu pause** (Échap) :

| Entrée | Solo | Coop |
|---|---|---|
| Reprendre | ✓ | ✓ |
| Options : touches (modifiables), souris, son, image | ✓ | ✓ |
| Sauvegarder | ✓ (manuel + automatique) | automatique, par le serveur |
| Charger une partie | ✓ | hôte seulement, depuis le menu principal |
| Nouvelle partie | ✓ | hôte seulement, depuis le menu principal |
| Quitter vers le menu / quitter le jeu | ✓ (sauvegarde avant de quitter) | ✓ (on se déconnecte, le monde continue) |

- **En solo, la pause arrête vraiment le monde** : le serveur local suspend ses ticks.
- **En coop, le monde continue** pendant qu'on est dans le menu : il n'y a pas de pause à plusieurs.
- Sauvegarde automatique : toutes les ~5 minutes et en quittant. Une sauvegarde = un monde (même format en solo
  et en coop, voir MODES.md), dans la base SQLite du serveur (T053).

**Touches par défaut** (toutes modifiables dans Options) :

| Action | Touche | Action | Touche |
|---|---|---|---|
| Avancer / reculer / côtés | ZQSD (AZERTY) ou WASD (QWERTY) | Inventaire | Tab |
| Courir | Maj | Carte | M |
| Sauter | Espace | Journal d'indices | J |
| Interagir, ramasser | E | Lampe torche | F |
| Attaquer | Clic gauche | Bloquer, viser | Clic droit |
| Esquiver | Ctrl | Pause | Échap |

## 6. Notifications

Messages courts, en haut à droite, qui n'interrompent jamais le jeu : 3 à la fois au maximum, ~4 secondes chacun,
regroupés quand ils se répètent (« +3 Bûche »).

- **Exploration** : nouvelle région découverte, carte à mettre à jour (avec une table proche), nouveau lieu.
- **Objets** : objet ramassé, inventaire plein, recette débloquée, outil presque cassé, pile faible.
- **Coop** : coéquipier connecté ou déconnecté, carte mise à jour par un coéquipier.
- **Système** : partie sauvegardée.

## 7. Animations de combat (Blender)

Faites sur les rigs des survivants (T014, T058), en **deux versions** : bras en vue subjective et corps entier
(vu par les autres en coop). Elles sont **visuelles** : le timing qui compte (moment où le coup porte, fenêtre de
blocage) est décidé par la simulation, pour rester juste malgré la latence.

| Arme | Animations |
|---|---|
| Mains nues | Coup de poing, bloquer, esquiver, être touché |
| Hache, pioche, pied de biche (une main) | Frapper (2 variantes), frapper un arbre ou un rocher, bloquer |
| Lance | Estoc, coup large, bloquer |
| Arc | Bander, tirer, ranger la flèche |
| Arbalète | Épauler, tirer, recharger (pied dans l'étrier) |
| Pistolet | Viser, tirer, recharger (barillet) |
| Fusil à pompe | Épauler, tirer, pomper, recharger cartouche par cartouche |

## Reste à décider

- Vitesses, coûts d'endurance et durées exactes (en jouant).
- Recettes chiffrées (T046) et fabrication des munitions.
- Rayon dans lequel on « mémorise » une région, et ce qu'on voit d'un point haut.
- Coffres et stockage partagé en coop.
