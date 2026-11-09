#include "odyssee/core/Config.hpp"
#include "odyssee/net/ClientConnection.hpp"

#include <doctest/doctest.h>

#include <chrono>

using namespace odyssee::net;
using namespace std::chrono_literals;
using State = ConnectionState;

namespace {

const ClientConnection::Clock::time_point t0{};

} // namespace

TEST_CASE("ClientConnection : poignée de main complète") {
    ClientConnection client("Teo");
    client.connect(t0);
    CHECK(client.state() == State::Requesting);
    CHECK(client.nextOutgoing(t0) == ControlMessage{ConnectRequest{"Teo"}});

    client.onMessage(Challenge{77}, t0 + 10ms);
    CHECK(client.state() == State::Answering);
    CHECK(client.nextOutgoing(t0 + 10ms) == ControlMessage{ChallengeResponse{77}});

    client.onMessage(Accepted{2}, t0 + 20ms);
    CHECK(client.state() == State::Connected);
    CHECK(client.playerId() == PlayerId{2});
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
    client.connect(t0);
    client.onMessage(Challenge{1}, t0);
    client.onMessage(Accepted{0}, t0);

    CHECK(client.nextOutgoing(t0 + odyssee::kKeepAliveInterval) == ControlMessage{KeepAlive{}});

    client.update(t0 + odyssee::kConnectionTimeout); // pile la limite : encore connecté
    CHECK(client.state() == State::Connected);
    client.update(t0 + odyssee::kConnectionTimeout + 1ms);
    CHECK(client.state() == State::TimedOut);
}

TEST_CASE("ClientConnection : déconnexion volontaire") {
    ClientConnection client("Teo");
    client.connect(t0);
    client.onMessage(Challenge{1}, t0);
    client.onMessage(Accepted{0}, t0);

    client.disconnect();
    CHECK(client.state() == State::Closed);
    for (int i = 0; i < 3; ++i) { // envoyé plusieurs fois, car il peut se perdre
        CHECK(client.nextOutgoing(t0) == ControlMessage{Disconnect{}});
    }
    CHECK_FALSE(client.nextOutgoing(t0 + 10s).has_value());
}

TEST_CASE("ClientConnection : les messages hors de propos sont ignorés") {
    ClientConnection client("Teo");
    client.onMessage(Accepted{1}, t0); // connexion pas encore lancée
    CHECK(client.state() == State::Idle);

    client.connect(t0);
    client.onMessage(Challenge{5}, t0);
    client.onMessage(Challenge{6}, t0); // second défi : on garde le premier
    CHECK(client.nextOutgoing(t0) == ControlMessage{ChallengeResponse{5}});
}
