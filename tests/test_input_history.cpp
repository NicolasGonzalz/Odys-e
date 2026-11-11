#include "odyssee/client/InputHistory.hpp"

#include "odyssee/net/Quantize.hpp"

#include <doctest/doctest.h>

using namespace odyssee;
using odyssee::client::InputHistory;

namespace {

/// Un input reconnaissable : son avancée vaut le numéro du tick (modulo 100).
net::InputCommand inputFor(net::Tick tick) {
    net::InputCommand command;
    command.moveForward = static_cast<std::int8_t>(tick % 100);
    return command;
}

void recordTicks(InputHistory& history, net::Tick first, net::Tick last) {
    for (net::Tick tick = first; tick <= last; ++tick) {
        history.record(tick, inputFor(tick));
    }
}

} // namespace

TEST_CASE("InputHistory : vide, le lot est vide") {
    const InputHistory history;
    CHECK(history.empty());
    CHECK(history.lastTick() == 0);
    CHECK(history.recentBatch().commands.empty());
}

TEST_CASE("InputHistory : le lot reprend les derniers inputs, avec le bon premier tick") {
    InputHistory history;
    recordTicks(history, 1, 30);

    const net::InputBatch batch = history.recentBatch(4);
    REQUIRE(batch.commands.size() == 4);
    CHECK(batch.firstTick == 27);
    CHECK(batch.commands.front() == inputFor(27));
    CHECK(batch.commands.back() == inputFor(30));
    CHECK(history.lastTick() == 30);
}

TEST_CASE("InputHistory : au début, le lot ne contient que ce qui existe") {
    InputHistory history;
    recordTicks(history, 1, 3);

    const net::InputBatch batch = history.recentBatch();
    CHECK(batch.firstTick == 1);
    CHECK(batch.commands.size() == 3);
}

TEST_CASE("InputHistory : au-delà de sa capacité, les plus vieux inputs sont oubliés") {
    InputHistory history;
    recordTicks(history, 1, InputHistory::kCapacity + 10);

    CHECK(history.size() == InputHistory::kCapacity);
    const net::InputBatch all = history.recentBatch(InputHistory::kCapacity);
    CHECK(all.firstTick == 11);
    CHECK(all.commands.front() == inputFor(11));
}

TEST_CASE("InputHistory : un tick qui ne suit pas repart de zéro (pas de trou dans un lot)") {
    InputHistory history;
    recordTicks(history, 1, 10);
    history.record(50, inputFor(50));

    CHECK(history.size() == 1);
    const net::InputBatch batch = history.recentBatch();
    CHECK(batch.firstTick == 50);
    CHECK(batch.commands.size() == 1);
}
