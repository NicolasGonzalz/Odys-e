#pragma once

#include "SipHash.hpp"

#include "odyssee/net/Address.hpp"

#include <chrono>
#include <cstdint>
#include <string_view>

namespace odyssee::server {

/// Jetons de défi **sans état** : le serveur ne retient rien quand il envoie un défi. Le jeton
/// est un code d'authentification (SipHash) de ce que la demande annonce, calculé avec un secret
/// que seul le serveur connaît ; quand la réponse revient, il suffit de le recalculer.
///
/// - Imprévisible : sans le secret, voir des milliers de jetons n'aide pas à deviner le suivant.
/// - Lié à l'adresse : un jeton reçu à une adresse ne vaut rien depuis une autre.
/// - Périssable : la période en cours entre dans le calcul ; un jeton vaut de 5 à 10 s.
/// - Insaturable : un flot de fausses demandes ne consomme aucune mémoire.
class ChallengeTokens {
  public:
    using Clock = std::chrono::steady_clock;

    /// Ce qu'une demande de connexion annonce, et que le jeton authentifie.
    struct Claim {
        net::Address address;
        std::uint64_t clientSalt = 0;
        std::string_view playerName;
    };

    explicit ChallengeTokens(const SipKey& key);

    /// Jeton à envoyer en défi à l'adresse de `claim`.
    [[nodiscard]] std::uint64_t issue(const Claim& claim, Clock::time_point now) const;

    /// Vrai si `token` est bien celui émis pour `claim`, et pas encore périmé.
    [[nodiscard]] bool verify(const Claim& claim, std::uint64_t token, Clock::time_point now) const;

  private:
    [[nodiscard]] std::uint64_t tokenFor(const Claim& claim, std::int64_t period) const;

    SipKey _key;
};

} // namespace odyssee::server
