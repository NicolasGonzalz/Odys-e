#pragma once

#include "odyssee/net/ControlMessages.hpp"

#include <cstdint>
#include <string>
#include <utility>

namespace odyssee::server {

/// Numéro d'une connexion acceptée : jamais réutilisé. Une place (PlayerId) est réutilisée quand
/// un joueur part ; la génération, elle, change à chaque nouvelle connexion.
using ConnectionGeneration = std::uint32_t;

/// Un joueur connecté, vu par le monde : sa place et la connexion qui l'occupe. Quand la
/// génération change pour une même place, c'est un autre joueur (ou le même qui s'est
/// reconnecté) : son état ne doit pas être repris.
struct ConnectedPlayer {
    ConnectedPlayer() = default;
    ConnectedPlayer(net::PlayerId playerId, ConnectionGeneration playerGeneration,
                    std::string playerName = {})
        : id(playerId), generation(playerGeneration), name(std::move(playerName)) {}

    net::PlayerId id = 0;
    ConnectionGeneration generation = 0;
    /// Nom choisi à la connexion : c'est lui qui retrouve la sauvegarde du joueur (T053).
    std::string name;

    friend bool operator==(const ConnectedPlayer&, const ConnectedPlayer&) = default;
};

} // namespace odyssee::server
