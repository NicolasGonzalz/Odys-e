#include "odyssee/net/ReliableReceiver.hpp"

#include <doctest/doctest.h>

#include <vector>

using namespace odyssee::net;

namespace {

ReliableMessage message(MessageId id) { return {id, {static_cast<std::byte>(id & 0xFF)}}; }

std::vector<int> ids(const std::vector<std::vector<std::byte>>& delivered) {
    std::vector<int> out;
    for (const auto& payload : delivered) {
        out.push_back(std::to_integer<int>(payload.at(0)));
    }
    return out;
}

} // namespace

TEST_CASE("ReliableReceiver : livraison dans l'ordre") {
    ReliableReceiver receiver;
    receiver.receive(message(0));
    receiver.receive(message(1));
    CHECK(ids(receiver.takeDeliverable()) == std::vector<int>{0, 1});
}

TEST_CASE("ReliableReceiver : un message en avance attend celui qui manque") {
    ReliableReceiver receiver;
    receiver.receive(message(0));
    receiver.receive(message(2)); // 1 pas encore arrivé
    CHECK(ids(receiver.takeDeliverable()) == std::vector<int>{0});
    CHECK(receiver.waitingCount() == 1);

    receiver.receive(message(1));
    CHECK(ids(receiver.takeDeliverable()) == std::vector<int>{1, 2});
    CHECK(receiver.waitingCount() == 0);
}

TEST_CASE("ReliableReceiver : les doublons sont ignorés") {
    ReliableReceiver receiver;
    receiver.receive(message(0));
    receiver.receive(message(0)); // doublon en attente
    CHECK(ids(receiver.takeDeliverable()) == std::vector<int>{0});

    receiver.receive(message(0)); // doublon déjà livré
    CHECK(receiver.takeDeliverable().empty());
}

TEST_CASE("ReliableReceiver : un message beaucoup trop en avance est refusé") {
    ReliableReceiver receiver;
    receiver.receive(message(5000));
    CHECK(receiver.waitingCount() == 0);
}
