# ADR 0004 — ECS maison pour le monde du serveur

- **Statut :** **accepté** (2026-11-13)
- **Tâche :** T043

## Contexte

Le serveur va gérer des entités de natures différentes : joueurs, ressources récoltables (avec
repousse), objets au sol, postes d'artisanat, créatures. Un ECS (entités + composants + systèmes)
évite une hiérarchie de classes et garde les données de chaque sorte d'entité serrées en mémoire.

Deux options : une bibliothèque reconnue (EnTT, un seul en-tête) ou un ECS écrit pour le projet.

## Décision

**ECS maison**, dans `server/src/ecs/` (le plugin Unreal n'en a pas besoin) :

- `Entity` : index + génération (un identifiant périmé ne désigne jamais l'entité qui reprend sa place).
- `ComponentPool<T>` : « sparse set » ; composants d'un type serrés dans un tableau, accès direct par
  index d'entité, retrait en échangeant avec le dernier.
- `Registry` : crée et détruit les entités, ajoute et lit les composants, et `each<A, B…>(fn)` parcourt
  les entités qui ont tous ces composants (à l'envers : un système peut détruire ce qu'il parcourt).
  Les types de composants sont numérotés sans RTTI.
- Les **systèmes** sont de simples fonctions appelées dans un ordre fixe par `GameWorld::step`.

## Raisons

- Quelques milliers d'entités au plus (4 joueurs, une zone chargée) : un sparse set simple suffit.
- ~200 lignes lisibles, aucune dépendance, des erreurs de compilation compréhensibles.
- Cohérent avec le reste du projet (UDP maison, simulation maison) : on maîtrise tout.

## Conséquences

- Pas de vues ni de groupes optimisés, pas de signaux : à ajouter si un besoin réel apparaît.
- Les numéros d'entité réseau (`net::EntityId`) restent distincts des `ecs::Entity` : les joueurs gardent
  les numéros 0 à 3 ; les autres entités en recevront un à leur entrée dans le monde (T044).
