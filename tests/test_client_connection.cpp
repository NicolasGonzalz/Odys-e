#include "odyssee/core/Config.hpp"
#include "odyssee/net/ClientConnection.hpp"

#include <doctest/doctest.h>

#include <chrono>

using namespace odyssee::net;
using namespace std::chrono_literals;
using State = ConnectionState;

namespace {

const ClientConnection::Clock::time_point t0{};

constexpr std::uint64_t kToken = 77;

/// Mène `client` jusqu'à l'état connecté, avec le jeton de session kToken.
void connectFully(ClientConnection& client) {
    client.connect(t0);
    client.onMessage(Challenge{kToken, client.clientSalt()}, t0);
    client.onMessage(Accepted{0, kToken}, t0);
    REQUIRE(client.state() == State::Connected);
}

} // namespace

TEST_CASE("ClientConnection : poignée de main complète") {
    ClientConnection client("Teo");
    client.connect(t0);
    CHECK(client.state() == State::Requesting);
    const auto salt = client.clientSalt();
    CHECK(client.nextOutgoing(t0) == ControlMessage{ConnectRequest{"Teo", salt}});
    CHECK_FALSE(client.sessionToken().has_value());

    client.onMessage(Challenge{77, salt}, t0 + 10ms);
    CHECK(client.state() == State::Answering);
    CHECK(client.nextOutgoing(t0 + 10ms) == ControlMessage{ChallengeResponse{77, salt, "Teo", ""}});

    CHECK_FALSE(client.serverWorldFingerprint());
    client.onMessage(Accepted{2, 77, 0xC0FFEE}, t0 + 20ms);
    CHECK(client.state() == State::Connected);
    CHECK(client.playerId() == PlayerId{2});
    CHECK(client.sessionToken() == SessionToken{77});
    CHECK(client.serverWorldFingerprint() == std::uint64_t{0xC0FFEE}); // carte du serveur
}

TEST_CASE("ClientConnection : la demande est renvoyée tant qu'il n'y a pas de réponse") {
    ClientConnection client("Teo");
    client.connect(t0);
    CHECK(client.nextOutgoing(t0).has_value());
    CHECK_FALSE(client.nextOutgoing(t0 + 100ms).has_value()); // trop tôt
    CHECK(client.nextOutgoing(t0 + odyssee::kHandshakeResendInterval).has_value());
}

TEST_CASE("ClientConnection : refus du serveur") {
    ClientConnection client("Teo");
    client.connect(t0);
    client.onMessage(Refused{RefuseReason::ServerFull}, t0 + 10ms);
    CHECK(client.state() == State::Refused);
    CHECK(client.refuseReason() == RefuseReason::ServerFull);
    CHECK_FALSE(client.nextOutgoing(t0 + 1s).has_value()); // on n'insiste pas
}

TEST_CASE("ClientConnection : serveur muet pendant la poignée de main") {
    ClientConnection client("Teo");
    client.connect(t0);
    client.update(t0 + odyssee::kHandshakeTimeout + 1ms);
    CHECK(client.state() == State::TimedOut);
}

TEST_CASE("ClientConnection : KeepAlive une fois connecté, puis timeout si le serveur se tait") {
    ClientConnection client("Teo");
    connectFully(client);

    CHECK(client.nextOutgoing(t0 + odyssee::kKeepAliveInterval) ==
          ControlMessage{KeepAlive{kToken}});

    client.update(t0 + odyssee::kConnectionTimeout); // pile la limite : encore connecté
    CHECK(client.state() == State::Connected);
    client.update(t0 + odyssee::kConnectionTimeout + 1ms);
    CHECK(client.state() == State::TimedOut);
}

TEST_CASE("ClientConnection : déconnexion volontaire") {
    ClientConnection client("Teo");
    connectFully(client);

    client.disconnect();
    CHECK(client.state() == State::Closed);
    for (int i = 0; i < 3; ++i) { // envoyé plusieurs fois, car il peut se perdre
        CHECK(client.nextOutgoing(t0) == ControlMessage{Disconnect{kToken}});
    }
    CHECK_FALSE(client.nextOutgoing(t0 + 10s).has_value());
}

TEST_CASE("ClientConnection : les messages hors de propos sont ignorés") {
    ClientConnection client("Teo");
    client.onMessage(Accepted{1, 0}, t0); // connexion pas encore lancée
    CHECK(client.state() == State::Idle);

    client.connect(t0);
    const auto salt = client.clientSalt();
    client.onMessage(Challenge{5, salt}, t0);
    client.onMessage(Challenge{6, salt}, t0); // second défi : on garde le premier
    CHECK(client.nextOutgoing(t0) == ControlMessage{ChallengeResponse{5, salt, "Teo", ""}});
}

TEST_CASE("ClientConnection : un défi ou une acceptation qui ne sont pas les nôtres sont ignorés") {
    ClientConnection client("Teo", 1234);
    client.connect(t0);
    const auto salt = client.clientSalt();

    client.onMessage(Challenge{5, salt + 1}, t0); // défi d'une demande qu'on n'a pas faite
    CHECK(client.state() == State::Requesting);
    client.onMessage(Accepted{0, 5}, t0); // acceptation sans défi : forgée
    CHECK(client.state() == State::Requesting);

    client.onMessage(Challenge{5, salt}, t0);
    client.onMessage(Accepted{0, 6}, t0); // mauvais jeton de session
    CHECK(client.state() == State::Answering);
    client.onMessage(Accepted{0, 5}, t0);
    CHECK(client.state() == State::Connected);
}

TEST_CASE("ClientConnection : un Disconnect ou un KeepAlive sans le jeton de session est ignoré") {
    ClientConnection client("Teo");
    connectFully(client);

    client.onMessage(Disconnect{kToken + 1}, t0 + 1s); // forgé : on reste connecté
    CHECK(client.state() == State::Connected);

    // Un faux KeepAlive ne prolonge pas la connexion : le serveur est jugé muet à l'heure prévue.
    client.onMessage(KeepAlive{kToken + 1}, t0 + 9s);
    client.update(t0 + odyssee::kConnectionTimeout + 1ms);
    CHECK(client.state() == State::TimedOut);
}

TEST_CASE("ClientConnection : le vrai KeepAlive prolonge, le vrai Disconnect ferme") {
    ClientConnection client("Teo");
    connectFully(client);

    client.onMessage(KeepAlive{kToken}, t0 + 9s);
    client.update(t0 + odyssee::kConnectionTimeout + 1ms);
    CHECK(client.state() == State::Connected);

    client.onMessage(Disconnect{kToken}, t0 + 10s);
    CHECK(client.state() == State::Closed);
}

TEST_CASE("ClientConnection : chaque tentative a un nouveau sel") {
    ClientConnection client("Teo", 42);
    client.connect(t0);
    const auto first = client.clientSalt();
    client.connect(t0 + 1s);
    CHECK(client.clientSalt() != first);
    CHECK(client.state() == State::Requesting);
}
