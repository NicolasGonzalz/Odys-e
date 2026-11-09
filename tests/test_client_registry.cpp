#include "ClientRegistry.hpp"

#include "odyssee/core/Config.hpp"

#include <doctest/doctest.h>

#include <chrono>

using namespace odyssee;
using namespace odyssee::net;
using odyssee::server::ClientRegistry;
using namespace std::chrono_literals;

namespace {

const ClientRegistry::Clock::time_point t0{};

Address client(std::uint16_t port) { return Address::loopback(port); }

/// Fait toute la poignée de main pour `from` et renvoie la réponse finale du serveur.
std::optional<ControlMessage> join(ClientRegistry& registry, const Address& from,
                                   ClientRegistry::Clock::time_point now = t0) {
    const auto challenge = registry.handle(from, ConnectRequest{"Joueur"}, now);
    const auto* token = challenge ? std::get_if<Challenge>(&*challenge) : nullptr;
    if (token == nullptr) {
        return challenge; // refus direct
    }
    return registry.handle(from, ChallengeResponse{token->token}, now);
}

} // namespace

TEST_CASE("ClientRegistry : un joueur se connecte et reçoit le numéro 0") {
    ClientRegistry registry(1);
    CHECK(join(registry, client(5000)) == ControlMessage{Accepted{0}});
    CHECK(registry.playerCount() == 1);
    CHECK(registry.playerAt(client(5000)) == PlayerId{0});
}

TEST_CASE("ClientRegistry : une mauvaise réponse au défi est ignorée") {
    ClientRegistry registry(1);
    const auto challenge = registry.handle(client(5000), ConnectRequest{"Teo"}, t0);
    REQUIRE(challenge.has_value());
    const auto token = std::get<Challenge>(*challenge).token;

    CHECK_FALSE(registry.handle(client(5000), ChallengeResponse{token + 1}, t0).has_value());
    CHECK_FALSE(registry.handle(client(6000), ChallengeResponse{token}, t0).has_value());
    CHECK(registry.playerCount() == 0);
}

TEST_CASE("ClientRegistry : demandes en double = mêmes réponses") {
    ClientRegistry registry(1);
    const auto first = registry.handle(client(5000), ConnectRequest{"Teo"}, t0);
    const auto again = registry.handle(client(5000), ConnectRequest{"Teo"}, t0 + 250ms);
    CHECK(first == again); // même défi

    REQUIRE(join(registry, client(5000)) == ControlMessage{Accepted{0}});
    CHECK(registry.handle(client(5000), ConnectRequest{"Teo"}, t0) ==
          ControlMessage{Accepted{0}}); // acceptation renvoyée
}

TEST_CASE("ClientRegistry : 4 joueurs maximum") {
    ClientRegistry registry(1);
    for (std::uint16_t i = 0; i < kMaxPlayers; ++i) {
        CHECK(join(registry, client(static_cast<std::uint16_t>(5000 + i))) ==
              ControlMessage{Accepted{static_cast<PlayerId>(i)}});
    }
    CHECK(join(registry, client(6000)) == ControlMessage{Refused{RefuseReason::ServerFull}});
}

TEST_CASE("ClientRegistry : pseudo invalide refusé") {
    ClientRegistry registry(1);
    CHECK(registry.handle(client(5000), ConnectRequest{""}, t0) ==
          ControlMessage{Refused{RefuseReason::InvalidName}});
}

TEST_CASE("ClientRegistry : une place libérée est réutilisée") {
    ClientRegistry registry(1);
    REQUIRE(join(registry, client(5000)) == ControlMessage{Accepted{0}});
    REQUIRE(join(registry, client(5001)) == ControlMessage{Accepted{1}});

    (void)registry.handle(client(5000), Disconnect{}, t0);
    CHECK(registry.playerCount() == 1);
    CHECK(join(registry, client(5002)) == ControlMessage{Accepted{0}}); // la place 0 est reprise
}

TEST_CASE("ClientRegistry : joueur silencieux retiré après le timeout") {
    ClientRegistry registry(1);
    REQUIRE(join(registry, client(5000)) == ControlMessage{Accepted{0}});
    REQUIRE(join(registry, client(5001)) == ControlMessage{Accepted{1}});

    // Le joueur 1 donne des nouvelles, le joueur 0 non.
    (void)registry.handle(client(5001), KeepAlive{}, t0 + 8s);

    const auto dropped = registry.dropInactive(t0 + kConnectionTimeout + 1ms);
    CHECK(dropped == std::vector<PlayerId>{0});
    CHECK(registry.playerCount() == 1);
}

TEST_CASE("ClientRegistry : poignée de main abandonnée oubliée") {
    ClientRegistry registry(1);
    const auto challenge = registry.handle(client(5000), ConnectRequest{"Teo"}, t0);
    const auto token = std::get<Challenge>(*challenge).token;

    (void)registry.dropInactive(t0 + kHandshakeTimeout + 1ms);
    CHECK_FALSE(registry.handle(client(5000), ChallengeResponse{token}, t0 + 6s).has_value());
}

TEST_CASE("ClientRegistry : KeepAlive envoyé à chaque joueur une fois par seconde") {
    ClientRegistry registry(1);
    REQUIRE(join(registry, client(5000)) == ControlMessage{Accepted{0}});
    CHECK(registry.takeKeepAliveTargets(t0 + 500ms).empty());
    CHECK(registry.takeKeepAliveTargets(t0 + kKeepAliveInterval).size() == 1);
    CHECK(registry.takeKeepAliveTargets(t0 + kKeepAliveInterval + 1ms).empty()); // déjà fait
}
