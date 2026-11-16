# Le bracelet de stase

> **Statut : concept validé (2026-11-16).** Remplace les écrans séparés d'inventaire, d'artisanat, de journal et
> de carte (GAMEPLAY.md §§ 2 à 4). Restent à décider : écran ou hologramme, raccourcis clavier (§ 6).

## 1. L'idée

Il n'y a **pas de menu d'inventaire**. Chaque survivant porte encore au poignet le **bracelet de sa capsule
de stase** Odyssée, une technologie d'avant la Floraison. Pour consulter ses affaires, son journal ou sa carte,
le personnage **lève le poignet** (animation en vue à la première personne) et le bracelet affiche ce qu'il faut.

- **Le temps continue** pendant qu'on regarde son bracelet : pas de pause, pas de menu. Ouvrir son bracelet en
  territoire hostile est un risque ; en coop, les autres voient le geste.
- **Recharge solaire** : pas de batterie à gérer. (Les piles restent pour la lampe torche.)
- **Toujours sur soi** : soudé au poignet, on le garde à la mort, comme l'équipement.
- Affichage **à décider** : petit écran sur le bracelet (on lève le bras, il remplit un tiers de la vue) ou
  projection holographique au-dessus du poignet (plus grand, plus lisible).

## 2. Ce que le bracelet explique dans l'histoire

Le bracelet donne une raison aux règles du jeu, sans texte d'explication :

| Règle | Raison dans le monde |
|---|---|
| Indice personnel lisible par son seul survivant (HISTOIRE.md § 4) | Les données sont **chiffrées pour son bracelet** |
| Version générique d'un indice personnel | Lue par un autre bracelet : donnée **corrompue**, à moitié lisible |
| Journal commun en coop | Les bracelets **se synchronisent** entre survivants proches |
| Souvenirs de carte à reporter (T074) | Le bracelet **enregistre** les lieux traversés ; on les **décharge** à la table de cartographie |
| Équipement gardé à la mort | Le bracelet ne se détache pas |

Évolution (v1) : après la Souche de Gaïa (zone 3), si le joueur a **respecté Gaïa**, de la mousse et de fines
racines poussent sur le bracelet ; l'affichage devient plus organique, aux couleurs de Gaïa. L'outil raconte la
relation du survivant avec elle.

## 3. Les indices : scannés ou branchés

Deux supports (clé `support` de chaque indice dans `data/story.txt`) :

- **Lettre** (`letter`) : notes, registres, affiches, graffitis, objets. On les **vise** et le bracelet les
  **scanne** (texte ou image).
- **Données** (`data`) : **clés USB** et **terminaux**, qu'on **branche** au bracelet. Ce sont les grands morceaux
  de l'histoire : ils comblent les trous de la frise.

Chaque indice porte une **date** (clé `date`, « 2073-10 » ou « 2074-10-17 »), qui le place sur la frise.

## 4. Les cinq onglets

| Onglet | Contenu | État du code |
|---|---|---|
| **Sac** | Inventaire (25 cases) et barre d'outils ; fiche de l'objet survolé ; glisser pour ranger | Serveur fait (T045) |
| **Fabrication** | Recettes possibles ici (à la main, ou au poste près duquel on se tient), ingrédients, durée | Serveur fait (T047) |
| **Survie** | Le **carnet de survie** : ce que le survivant a *appris* en faisant. Une entrée s'ajoute la première fois qu'on récolte, chasse, fabrique ou mange quelque chose (« Oiseau : plumes, viande d'oiseau », « Peau : se travaille en tissu à l'établi »). Et ses besoins vitaux en détail | À faire (T078) |
| **Histoire** | La **frise chronologique à trous** (2071 → 2074 → réveil en 2201) : chaque indice se place à sa date, les périodes encore vides se voient. Les indices lus, et les souvenirs du survivant | Serveur fait (T049) ; frise à faire (T080) |
| **Carte** | La carte commune et ses souvenirs à décharger (T074), sa position et celle des coéquipiers, la **boussole** vers le prochain point d'intérêt (ancien GPS, T076) | Serveur fait (T074) |

## 5. Le carnet de survie (T078)

- Le serveur retient, par joueur, ses **premières fois** : chaque ressource récoltée, objet fabriqué, objet mangé,
  et (plus tard) chaque animal chassé ou dépecé. Il les envoie au client et les sauvegarde avec le joueur.
- Le client affiche l'entrée correspondante, écrite à partir des données (`data/resources.txt`,
  `data/recipes.txt`, `data/items.txt`) : rien à rédiger à la main pour l'essentiel.
- Chaque survivant tient **son** carnet (pas de synchronisation) : apprendre fait partie de son histoire.

## 6. Raccourcis clavier (à décider)

Proposition : le bracelet s'ouvre avec **Tab** sur le dernier onglet ouvert ; des raccourcis l'ouvrent
directement sur un onglet (**I** sac, **C** fabrication, **J** histoire, **M** carte). Le geste du poignet est le
même dans tous les cas. À essayer en jeu avant de trancher.
