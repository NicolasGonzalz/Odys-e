#include "ClientRegistry.hpp"

#include "odyssee/core/Config.hpp"

#include <doctest/doctest.h>

#include <chrono>
#include <set>

using namespace odyssee;
using namespace odyssee::net;
using odyssee::server::ClientRegistry;
using namespace std::chrono_literals;

namespace {

using TimePoint = ClientRegistry::Clock::time_point;

const TimePoint t0{};

Address client(std::uint16_t port) { return Address::loopback(port); }

/// Demande un défi pour `from` et renvoie son jeton (0 si le serveur n'a pas envoyé de défi).
std::uint64_t challengeFor(ClientRegistry& registry, const Address& from, std::uint64_t salt,
                           TimePoint now = t0) {
    const auto reply = registry.handle(from, ConnectRequest{"Joueur", salt}, now);
    const auto* challenge = reply ? std::get_if<Challenge>(&*reply) : nullptr;
    return challenge != nullptr ? challenge->token : 0;
}

/// Fait toute la poignée de main pour `from` et renvoie la réponse finale du serveur.
std::optional<ControlMessage> join(ClientRegistry& registry, const Address& from,
                                   std::uint64_t salt = 1, TimePoint now = t0) {
    const auto reply = registry.handle(from, ConnectRequest{"Joueur", salt}, now);
    const auto* challenge = reply ? std::get_if<Challenge>(&*reply) : nullptr;
    if (challenge == nullptr) {
        return reply; // refus direct
    }
    return registry.handle(from, ChallengeResponse{challenge->token, salt, "Joueur"}, now);
}

/// Jeton de session du joueur connecté à `address`.
SessionToken tokenOf(const ClientRegistry& registry, const Address& address) {
    for (const auto& player : registry.players()) {
        if (player.address == address) {
            return player.sessionToken;
        }
    }
    FAIL("aucun joueur à cette adresse");
    return 0;
}

PlayerId idOf(const std::optional<ControlMessage>& reply) {
    REQUIRE(reply.has_value());
    REQUIRE(std::holds_alternative<Accepted>(*reply));
    return std::get<Accepted>(*reply).playerId;
}

} // namespace

TEST_CASE("ClientRegistry : un joueur se connecte et reçoit le numéro 0") {
    ClientRegistry registry(1);
    const auto reply = join(registry, client(5000));
    CHECK(idOf(reply) == 0);
    CHECK(registry.playerCount() == 1);
    CHECK(registry.playerAt(client(5000)) == PlayerId{0});
    // Le jeton de session annoncé au client est celui que le serveur exigera ensuite.
    CHECK(std::get<Accepted>(*reply).sessionToken == tokenOf(registry, client(5000)));
}

TEST_CASE("ClientRegistry : une mauvaise réponse au défi est ignorée") {
    ClientRegistry registry(1);
    const auto token = challengeFor(registry, client(5000), 9);
    REQUIRE(token != 0);

    const auto answer = [&](const Address& from, const ChallengeResponse& response) {
        return registry.handle(from, response, t0).has_value();
    };
    CHECK_FALSE(answer(client(5000), {token + 1, 9, "Joueur"})); // mauvais jeton
    CHECK_FALSE(answer(client(6000), {token, 9, "Joueur"}));     // autre adresse
    CHECK_FALSE(answer(client(5000), {token, 10, "Joueur"}));    // autre sel
    CHECK_FALSE(answer(client(5000), {token, 9, "Autre"}));      // autre pseudo
    CHECK(registry.playerCount() == 0);

    CHECK(answer(client(5000), {token, 9, "Joueur"}));
    CHECK(registry.playerCount() == 1);
}

TEST_CASE("ClientRegistry : demandes en double = mêmes réponses") {
    ClientRegistry registry(1);
    const auto first = registry.handle(client(5000), ConnectRequest{"Teo", 3}, t0);
    const auto again = registry.handle(client(5000), ConnectRequest{"Teo", 3}, t0 + 250ms);
    CHECK(first == again); // même défi

    // Acceptation perdue : le client renvoie sa réponse, le serveur renvoie la même acceptation.
    const auto token = std::get<Challenge>(*first).token;
    const ChallengeResponse response{token, 3, "Teo"};
    const auto accepted = registry.handle(client(5000), response, t0);
    CHECK(accepted == ControlMessage{Accepted{0, token}});
    CHECK(registry.handle(client(5000), response, t0 + 250ms) == accepted);
    CHECK(registry.playerCount() == 1);
}

TEST_CASE("ClientRegistry : les jetons de défi sont imprévisibles et propres à chaque demande") {
    ClientRegistry registry(1);
    ClientRegistry other(2); // autre secret

    std::set<std::uint64_t> tokens;
    for (std::uint64_t salt = 0; salt < 200; ++salt) {
        tokens.insert(challengeFor(registry, client(5000), salt));
    }
    CHECK(tokens.size() == 200); // un jeton par sel

    const auto token = challengeFor(registry, client(5000), 1);
    CHECK(token == challengeFor(registry, client(5000), 1));  // reproductible (sans état)
    CHECK(token != challengeFor(registry, client(5001), 1));  // lié à l'adresse
    CHECK(token != challengeFor(other, client(5000), 1));     // lié au secret du serveur
    CHECK(token != challengeFor(registry, client(5000), 1, t0 + kHandshakeTimeout)); // périssable
}

TEST_CASE("ClientRegistry : un flot de fausses demandes ne retient rien et ne bloque personne") {
    ClientRegistry registry(1);
    for (std::uint32_t i = 0; i < 20'000; ++i) {
        const Address fake{0x0A000000u + i, static_cast<std::uint16_t>(1024 + i % 60'000)};
        CHECK(challengeFor(registry, fake, i) != 0);
    }
    CHECK(registry.playerCount() == 0);
    CHECK(idOf(join(registry, client(5000))) == 0); // un vrai joueur entre toujours
}

TEST_CASE("ClientRegistry : 4 joueurs maximum") {
    ClientRegistry registry(1);
    for (std::uint16_t i = 0; i < kMaxPlayers; ++i) {
        CHECK(idOf(join(registry, client(static_cast<std::uint16_t>(5000 + i)))) == i);
    }
    CHECK(join(registry, client(6000)) == ControlMessage{Refused{RefuseReason::ServerFull}});
}

TEST_CASE("ClientRegistry : plafond de joueurs par adresse IP") {
    ClientRegistry registry(1, ClientRegistry::Limits{2});
    const Address elsewhere{0xC0A80014u, 5000}; // 192.168.0.20

    CHECK(idOf(join(registry, client(5000))) == 0);
    CHECK(idOf(join(registry, client(5001))) == 1);
    CHECK(join(registry, client(5002)) ==
          ControlMessage{Refused{RefuseReason::TooManyFromAddress}});
    CHECK(idOf(join(registry, elsewhere)) == 2); // une autre IP n'est pas concernée

    // Le plafond vaut aussi pour un défi obtenu avant que l'adresse soit pleine.
    ClientRegistry late(1, ClientRegistry::Limits{1});
    const auto token = challengeFor(late, client(5001), 4);
    REQUIRE(idOf(join(late, client(5000))) == 0);
    CHECK(late.handle(client(5001), ChallengeResponse{token, 4, "Joueur"}, t0) ==
          ControlMessage{Refused{RefuseReason::TooManyFromAddress}});
    CHECK(late.playerCount() == 1);
}

TEST_CASE("ClientRegistry : pseudo invalide refusé") {
    ClientRegistry registry(1);
    CHECK(registry.handle(client(5000), ConnectRequest{"", 1}, t0) ==
          ControlMessage{Refused{RefuseReason::InvalidName}});
}

TEST_CASE("ClientRegistry : une place libérée est réutilisée, avec une nouvelle génération") {
    ClientRegistry registry(1);
    REQUIRE(idOf(join(registry, client(5000))) == 0);
    REQUIRE(idOf(join(registry, client(5001))) == 1);
    const auto firstGeneration = registry.players().front().generation;

    (void)registry.handle(client(5000), Disconnect{tokenOf(registry, client(5000))}, t0);
    CHECK(registry.playerCount() == 1);
    CHECK(idOf(join(registry, client(5002))) == 0); // la place 0 est reprise

    for (const auto& player : registry.players()) {
        if (player.address == client(5002)) {
            CHECK(player.generation > firstGeneration);
        }
    }
}

TEST_CASE("ClientRegistry : Disconnect et KeepAlive exigent le jeton de session") {
    ClientRegistry registry(1);
    REQUIRE(idOf(join(registry, client(5000))) == 0);
    const auto token = tokenOf(registry, client(5000));

    // Adresse usurpée, jeton deviné : rien ne se passe.
    (void)registry.handle(client(5000), Disconnect{token + 1}, t0);
    (void)registry.handle(client(5000), Disconnect{0}, t0);
    CHECK(registry.playerCount() == 1);

    // Un faux KeepAlive ne maintient pas le joueur en vie.
    (void)registry.handle(client(5000), KeepAlive{token + 1}, t0 + 8s);
    CHECK(registry.dropInactive(t0 + kConnectionTimeout + 1ms) == std::vector<PlayerId>{0});
}

TEST_CASE("ClientRegistry : une demande de connexion ne touche pas au joueur déjà connecté") {
    ClientRegistry registry(1);
    REQUIRE(idOf(join(registry, client(5000))) == 0);
    const auto token = tokenOf(registry, client(5000));
    const auto generation = registry.players().front().generation;

    // N'importe qui peut envoyer une demande avec l'adresse d'un joueur : elle ne vaut qu'un
    // défi, et ne prolonge pas non plus la connexion.
    const auto reply = registry.handle(client(5000), ConnectRequest{"Intrus", 99}, t0 + 8s);
    REQUIRE(reply.has_value());
    CHECK(std::holds_alternative<Challenge>(*reply));
    CHECK(registry.playerCount() == 1);
    CHECK(tokenOf(registry, client(5000)) == token);
    CHECK(registry.players().front().generation == generation);
    CHECK(registry.dropInactive(t0 + kConnectionTimeout + 1ms).size() == 1);
}

TEST_CASE("ClientRegistry : reconnexion depuis la même adresse = nouvelle session") {
    ClientRegistry registry(1);
    REQUIRE(idOf(join(registry, client(5000), 1)) == 0);
    const auto oldToken = tokenOf(registry, client(5000));
    const auto oldGeneration = registry.players().front().generation;

    // Le client redémarre sur le même port : nouvelle poignée de main complète (nouveau sel).
    const auto reply = join(registry, client(5000), 2, t0 + 1s);
    CHECK(idOf(reply) == 0);
    CHECK(registry.playerCount() == 1);
    const auto newToken = tokenOf(registry, client(5000));
    CHECK(newToken != oldToken);
    CHECK(registry.players().front().generation != oldGeneration);

    // L'ancien jeton ne vaut plus rien, ni pour agir, ni pour revenir (réponse rejouée).
    (void)registry.handle(client(5000), Disconnect{oldToken}, t0 + 1s);
    CHECK(registry.playerCount() == 1);
    CHECK_FALSE(
        registry.handle(client(5000), ChallengeResponse{oldToken, 1, "Joueur"}, t0 + 1s)
            .has_value());
    CHECK(tokenOf(registry, client(5000)) == newToken);
}

TEST_CASE("ClientRegistry : même plein, un joueur peut se reconnecter depuis son adresse") {
    ClientRegistry registry(1);
    for (std::uint16_t i = 0; i < kMaxPlayers; ++i) {
        REQUIRE(idOf(join(registry, client(static_cast<std::uint16_t>(5000 + i)))) == i);
    }
    CHECK(idOf(join(registry, client(5002), 2)) == 2); // il retrouve sa place
    CHECK(registry.playerCount() == kMaxPlayers);
}

TEST_CASE("ClientRegistry : la réponse d'une session terminée ne recrée pas de joueur") {
    ClientRegistry registry(1);
    const auto token = challengeFor(registry, client(5000), 5);
    const ChallengeResponse response{token, 5, "Joueur"};
    REQUIRE(registry.handle(client(5000), response, t0).has_value());

    (void)registry.handle(client(5000), Disconnect{token}, t0 + 1s);
    REQUIRE(registry.playerCount() == 0);

    // Un doublon de la réponse arrive après le départ (ou est rejoué) : pas de joueur fantôme.
    CHECK_FALSE(registry.handle(client(5000), response, t0 + 2s).has_value());
    CHECK(registry.playerCount() == 0);
}

TEST_CASE("ClientRegistry : joueur silencieux retiré après le timeout") {
    ClientRegistry registry(1);
    REQUIRE(idOf(join(registry, client(5000))) == 0);
    REQUIRE(idOf(join(registry, client(5001))) == 1);

    // Le joueur 1 donne des nouvelles, le joueur 0 non.
    (void)registry.handle(client(5001), KeepAlive{tokenOf(registry, client(5001))}, t0 + 8s);

    const auto dropped = registry.dropInactive(t0 + kConnectionTimeout + 1ms);
    CHECK(dropped == std::vector<PlayerId>{0});
    CHECK(registry.playerCount() == 1);
}

TEST_CASE("ClientRegistry : un défi périmé n'est plus accepté") {
    ClientRegistry registry(1);
    const auto token = challengeFor(registry, client(5000), 1);
    const ChallengeResponse response{token, 1, "Joueur"};

    // Valable au moins le temps d'une poignée de main, au plus deux fois ce temps.
    CHECK_FALSE(registry.handle(client(5000), response, t0 + 2 * kHandshakeTimeout).has_value());
    CHECK(registry.playerCount() == 0);
    CHECK(registry.handle(client(5000), response, t0 + kHandshakeTimeout - 1ms).has_value());
}

TEST_CASE("ClientRegistry : KeepAlive envoyé à chaque joueur une fois par seconde") {
    ClientRegistry registry(1);
    REQUIRE(idOf(join(registry, client(5000))) == 0);
    CHECK(registry.takeKeepAliveTargets(t0 + 500ms).empty());
    const auto targets = registry.takeKeepAliveTargets(t0 + kKeepAliveInterval);
    REQUIRE(targets.size() == 1);
    CHECK(targets.front().sessionToken == tokenOf(registry, client(5000)));
    CHECK(registry.takeKeepAliveTargets(t0 + kKeepAliveInterval + 1ms).empty()); // déjà fait
}
