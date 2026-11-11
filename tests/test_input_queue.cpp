#include "InputQueue.hpp"

#include <doctest/doctest.h>

#include <cstdint>

using namespace odyssee;
using odyssee::server::InputQueue;

namespace {

/// Lot de `count` inputs de ticks consécutifs ; l'input du tick t a moveRight = t % 100.
net::InputBatch batch(net::Tick firstTick, std::size_t count) {
    net::InputBatch result{firstTick, {}};
    for (std::size_t i = 0; i < count; ++i) {
        net::InputCommand command;
        command.moveRight = static_cast<std::int8_t>((firstTick + i) % 100);
        result.commands.push_back(command);
    }
    return result;
}

} // namespace

TEST_CASE("InputQueue : un input par appel, dans l'ordre des ticks") {
    InputQueue queue;
    queue.push(batch(10, 3));
    REQUIRE(queue.size() == 3);
    CHECK(queue.pop()->tick == 10);
    CHECK(queue.pop()->tick == 11);
    CHECK(queue.pop()->tick == 12);
    CHECK_FALSE(queue.pop().has_value());
}

TEST_CASE("InputQueue : les inputs répétés par redondance ne sont gardés qu'une fois") {
    InputQueue queue;
    queue.push(batch(10, 3)); // 10, 11, 12
    queue.push(batch(11, 3)); // 11, 12 déjà vus ; 13 nouveau
    CHECK(queue.size() == 4);
    queue.push(batch(5, 2)); // trop vieux
    CHECK(queue.size() == 4);

    const auto first = queue.pop();
    REQUIRE(first.has_value());
    CHECK(first->tick == 10);
    CHECK(first->command.moveRight == 10);
}

TEST_CASE("InputQueue : un trou (perte malgré la redondance) est simplement sauté") {
    InputQueue queue;
    queue.push(batch(10, 1));
    queue.push(batch(14, 1)); // 11 à 13 perdus
    CHECK(queue.pop()->tick == 10);
    CHECK(queue.pop()->tick == 14);
}

TEST_CASE("InputQueue : le retard est borné, les plus anciens sont sautés") {
    InputQueue queue;
    queue.push(batch(100, 16)); // 100 à 115
    REQUIRE(queue.size() == InputQueue::kMaxBacklog);
    CHECK(queue.pop()->tick == 116 - InputQueue::kMaxBacklog);
}
