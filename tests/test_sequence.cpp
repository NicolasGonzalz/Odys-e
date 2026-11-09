#include "odyssee/net/Sequence.hpp"

#include <doctest/doctest.h>

using odyssee::net::isMoreRecent;
using odyssee::net::sequenceDistance;

TEST_CASE("Sequence : comparaison simple") {
    CHECK(isMoreRecent(2, 1));
    CHECK_FALSE(isMoreRecent(1, 2));
    CHECK_FALSE(isMoreRecent(7, 7)); // un numéro n'est pas plus récent que lui-même
}

TEST_CASE("Sequence : le rebouclage 65535 → 0 est géré") {
    CHECK(isMoreRecent(0, 65535));
    CHECK(isMoreRecent(2, 65534));
    CHECK_FALSE(isMoreRecent(65534, 2));
    CHECK(sequenceDistance(2, 65534) == 4);
}

TEST_CASE("Sequence : au-delà d'un demi-tour, l'ordre s'inverse") {
    CHECK(isMoreRecent(32767, 0));       // juste sous le demi-tour : plus récent
    CHECK_FALSE(isMoreRecent(32768, 0)); // demi-tour exact : considéré comme plus ancien
}
