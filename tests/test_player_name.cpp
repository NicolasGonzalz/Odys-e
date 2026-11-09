#include "odyssee/net/PlayerName.hpp"

#include <doctest/doctest.h>

using odyssee::net::isValidPlayerName;

TEST_CASE("PlayerName : pseudos valides") {
    CHECK(isValidPlayerName("Teo"));
    CHECK(isValidPlayerName("Mara Velin"));
    CHECK(isValidPlayerName("Élodie"));           // accents (UTF-8) acceptés
    CHECK(isValidPlayerName("abcdefghijklmnop")); // 16 caractères, la limite
}

TEST_CASE("PlayerName : pseudos invalides") {
    CHECK_FALSE(isValidPlayerName(""));
    CHECK_FALSE(isValidPlayerName("   "));
    CHECK_FALSE(isValidPlayerName("abcdefghijklmnopq")); // 17 caractères
    CHECK_FALSE(isValidPlayerName("Teo\nMara"));         // caractère de contrôle
    CHECK_FALSE(isValidPlayerName("Te\to"));
}
