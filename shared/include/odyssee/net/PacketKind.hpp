#pragma once

#include <cstdint>

namespace odyssee::net {

/// Type d'un paquet, écrit juste après l'en-tête.
///
/// Les valeurs sont **figées** : un client d'une autre version doit au moins pouvoir lire
/// un refus (Refused) pour afficher « mauvaise version ». On ajoute, on ne renumérote jamais.
enum class PacketKind : std::uint8_t {
    ConnectRequest = 1,    ///< client → serveur : « je veux me connecter »
    Challenge = 2,         ///< serveur → client : défi à renvoyer
    ChallengeResponse = 3, ///< client → serveur : défi renvoyé
    Accepted = 4,          ///< serveur → client : connecté, voici ton numéro de joueur
    Refused = 5,           ///< serveur → client : connexion refusée (avec la raison)
    Disconnect = 6,        ///< dans les deux sens : départ volontaire
    KeepAlive = 7,         ///< dans les deux sens : « je suis toujours là »
};

/// Vrai si `value` correspond à un PacketKind connu.
[[nodiscard]] constexpr bool isKnownPacketKind(std::uint8_t value) {
    return value >= static_cast<std::uint8_t>(PacketKind::ConnectRequest) &&
           value <= static_cast<std::uint8_t>(PacketKind::KeepAlive);
}

} // namespace odyssee::net
