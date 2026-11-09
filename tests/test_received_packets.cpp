#include "odyssee/net/ReceivedPackets.hpp"

#include <doctest/doctest.h>

using odyssee::net::Arrival;
using odyssee::net::ReceivedPackets;

TEST_CASE("ReceivedPackets : paquets consécutifs") {
    ReceivedPackets received;
    CHECK(received.record(10) == Arrival::New);
    CHECK(received.record(11) == Arrival::New);
    CHECK(received.record(12) == Arrival::New);
    CHECK(received.ack() == 12);
    CHECK(received.ackBits() == 0b11); // 11 et 10 reçus
}

TEST_CASE("ReceivedPackets : un trou est visible dans ackBits") {
    ReceivedPackets received;
    received.record(97);
    received.record(99); // 98 perdu
    received.record(100);
    CHECK(received.ack() == 100);
    CHECK(received.ackBits() == 0b101); // 99 reçu, 98 absent, 97 reçu
}

TEST_CASE("ReceivedPackets : un paquet en retard comble le trou") {
    ReceivedPackets received;
    received.record(97);
    received.record(99);
    received.record(100);
    CHECK(received.record(98) == Arrival::New);
    CHECK(received.ack() == 100); // le plus récent ne change pas
    CHECK(received.ackBits() == 0b111);
}

TEST_CASE("ReceivedPackets : doublons détectés") {
    ReceivedPackets received;
    received.record(5);
    received.record(6);
    CHECK(received.record(6) == Arrival::Duplicate); // le plus récent
    CHECK(received.record(5) == Arrival::Duplicate); // un plus ancien
}

TEST_CASE("ReceivedPackets : trop vieux pour être jugé") {
    ReceivedPackets received;
    received.record(100);
    CHECK(received.record(68) == Arrival::New);    // 32 de retard : encore dans la fenêtre
    CHECK(received.record(67) == Arrival::TooOld); // 33 de retard : hors fenêtre
}

TEST_CASE("ReceivedPackets : grands sauts en avant") {
    ReceivedPackets received;
    received.record(0);
    received.record(32); // saut de 32 : l'ancien passe dans la dernière case
    CHECK(received.ackBits() == (1u << 31));

    received.record(100); // saut de 68 : tout l'historique sort de la fenêtre
    CHECK(received.ackBits() == 0);
}

TEST_CASE("ReceivedPackets : fonctionne à travers le rebouclage") {
    ReceivedPackets received;
    received.record(65534);
    received.record(65535);
    CHECK(received.record(0) == Arrival::New);
    CHECK(received.record(1) == Arrival::New);
    CHECK(received.ack() == 1);
    CHECK(received.ackBits() == 0b111); // 0, 65535 et 65534
    CHECK(received.record(65535) == Arrival::Duplicate);
}
