#include "odyssee/net/Acknowledgements.hpp"
#include "odyssee/net/ReceivedPackets.hpp"

#include <doctest/doctest.h>

#include <vector>

using odyssee::net::acknowledgedSequences;
using odyssee::net::Sequence;

TEST_CASE("Acknowledgements : décodage de ack + ackBits") {
    const auto acked = acknowledgedSequences(100, 0b101);
    CHECK(acked == std::vector<Sequence>{100, 99, 97});
}

TEST_CASE("Acknowledgements : rebouclage sous zéro") {
    const auto acked = acknowledgedSequences(1, 0b11);
    CHECK(acked == std::vector<Sequence>{1, 0, 65535});
}

TEST_CASE("Acknowledgements : les 33 paquets confirmés au maximum") {
    CHECK(acknowledgedSequences(500, 0xFFFFFFFFu).size() == 33);
}

TEST_CASE("Acknowledgements : ce que l'un enregistre, l'autre le décode") {
    odyssee::net::ReceivedPackets received;
    for (const Sequence s : std::vector<Sequence>{10, 11, 13, 14}) { // 12 perdu
        received.record(s);
    }
    const auto acked = acknowledgedSequences(received.ack(), received.ackBits());
    CHECK(acked == std::vector<Sequence>{14, 13, 11, 10});
}
