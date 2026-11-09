# Équipe & façon de travailler

## Rôles

| Domaine | Nicolas | Coéquipier |
|---|---|---|
| Serveur C++, protocole, réseau | ✅ responsable | |
| Simulation partagée (`shared/`), persistance, outils | ✅ responsable | |
| Infrastructure : CI, homelab, déploiement | ✅ responsable | |
| Plugin Unreal `OdysseeNet` (pont réseau/simulation) | ✅ responsable | relit l'intégration |
| Gameplay client : **logique** (réseau, prédiction, état synchronisé) | ✅ | |
| Gameplay client : **présentation** (animations, UI, effets, sons) | | ✅ |
| Contrôleur 1re personne, UI d'inventaire | ensemble : logique Nicolas, ressenti et visuel coéquipier | ensemble |
| Figma : concepts, plan des zones, maquettes d'UI | | ✅ responsable |
| Blender : personnages, décors, animations | | ✅ responsable |
| Cartes et level design dans Unreal | | ✅ responsable |
| Lore, game design, playtests | ensemble | ensemble |

Chaque tâche du [planning](PLANNING.md) indique **qui** en est responsable.

**Les rôles sont interchangeables** : chacun peut reprendre une tâche de l'autre (absence, retard, envie).
Le « qui » du planning indique le responsable par défaut, pas une exclusivité : il suffit de se réassigner l'issue.

## Postes de travail

- **Nicolas** : Linux pour le serveur et `shared/` ; Windows (dual-boot) pour l'éditeur Unreal et les tests du client.
  Le client de debug raylib permet de tester le serveur sous Linux sans lancer Unreal.
- **Coéquipier** : Windows, Unreal Engine (version figée du projet), Blender, Figma.

## Git

- **Une tâche = une branche = une Pull Request**, nommée d'après l'ID (`t021-paquet-header`). L'autre relit avant fusion.
- **Git LFS** pour tous les fichiers binaires (voir `.gitattributes`). Avant de modifier une carte ou un asset Unreal :
  `git lfs lock <fichier>`, puis `git lfs unlock` après la fusion.
- On ne fusionne jamais sur `main` avec une CI rouge.

## Rituels

- **Lundi** : objectifs de la semaine, chacun déplace ses issues en « En cours ».
- **Dimanche** : bilan rapide (fait, bloqué, à décaler).
- **Fin de jalon** : démo jouée ensemble, post d'avancement.
