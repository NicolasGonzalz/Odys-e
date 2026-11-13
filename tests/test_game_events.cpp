#include "odyssee/net/ByteBuffer.hpp"
#include "odyssee/net/GameEvents.hpp"

#include <doctest/doctest.h>

#include <cstddef>
#include <vector>

using namespace odyssee::net;

TEST_CASE("GameEvents : aller-retour de l'inventaire et d'une demande de déplacement") {
    InventoryContents contents;
    contents.slots[0] = {3, 12};
    contents.slots[24] = {41, 1};
    for (const GameEvent& event : {GameEvent{contents}, GameEvent{MoveItem{2, 17}}}) {
        const auto decoded = decodeGameEvent(encodeGameEvent(event));
        REQUIRE(decoded);
        CHECK(*decoded == event);
    }
}

TEST_CASE("GameEvents : messages invalides refusés") {
    auto bytes = encodeGameEvent(MoveItem{0, 1});
    bytes[2] = std::byte{kInventorySlots}; // case hors limites
    CHECK_FALSE(decodeGameEvent(bytes));

    bytes = encodeGameEvent(MoveItem{0, 1});
    bytes.push_back(std::byte{0}); // octet en trop
    CHECK_FALSE(decodeGameEvent(bytes));

    InventoryContents contents;
    contents.slots[0] = {5, 0}; // un objet dans une case vide
    CHECK_FALSE(decodeGameEvent(encodeGameEvent(contents)));

    CHECK_FALSE(decodeGameEvent(std::vector<std::byte>{std::byte{99}})); // type inconnu
    CHECK_FALSE(decodeGameEvent(std::vector<std::byte>{}));
    bytes = encodeGameEvent(InventoryContents{});
    bytes.pop_back(); // tronqué
    CHECK_FALSE(decodeGameEvent(bytes));
}
