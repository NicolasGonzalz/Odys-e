#pragma once

#include "odyssee/net/GameMessages.hpp"
#include "odyssee/sim/Movement.hpp"

namespace odyssee::sim {

/// Passage entre l'état de simulation d'un joueur et ce qu'en dit un snapshot. Le serveur
/// l'utilise pour remplir le snapshot, le client pour reprendre sa prédiction (réconciliation) :
/// les deux sens doivent rester exactement réciproques.

/// Ce que tous les joueurs voient de lui.
[[nodiscard]] net::EntityState toEntity(net::EntityId id, const PlayerState& state);

/// Ce que lui seul reçoit en plus (vitesse, endurance).
[[nodiscard]] net::OwnMotion toOwnMotion(const PlayerState& state);

/// L'état complet reconstitué à partir de son entité et de `self`. L'inclinaison n'est pas
/// transmise (elle ne joue pas sur le mouvement) : elle vaut 0, et le prochain pas la reprend de
/// l'input.
[[nodiscard]] PlayerState restore(const net::EntityState& entity, const net::OwnMotion& own);

} // namespace odyssee::sim
