#include "odyssee/net/InviteCode.hpp"

#include <doctest/doctest.h>

#include <string>

using namespace odyssee::net;

TEST_CASE("Code d'invitation : majuscules, tirets et espaces ne comptent pas") {
    CHECK(normalizeInviteCode("ody-7kq2") == "ODY7KQ2");
    CHECK(normalizeInviteCode(" ODY 7KQ2 ") == "ODY7KQ2");
    CHECK(inviteCodeMatches("ODY7KQ2", "ody-7kq2"));
    CHECK(inviteCodeMatches("ODY7KQ2", "Ody 7Kq2"));
}

TEST_CASE("Code d'invitation : un code faux, plus court ou plus long ne passe pas") {
    CHECK_FALSE(inviteCodeMatches("ODY7KQ2", "ODY7KQ3"));
    CHECK_FALSE(inviteCodeMatches("ODY7KQ2", "ODY7KQ"));
    CHECK_FALSE(inviteCodeMatches("ODY7KQ2", "ODY7KQ22"));
    CHECK_FALSE(inviteCodeMatches("ODY7KQ2", ""));
    // Un code trop long dont le début est le bon : refusé, même au-delà de 32 caractères.
    CHECK_FALSE(inviteCodeMatches("ODY7KQ2", "ODY7KQ2" + std::string(40, 'A')));
}

TEST_CASE("Code d'invitation : lettres et chiffres seulement, 32 au plus") {
    CHECK(isValidInviteCode("ODY-7KQ2"));
    CHECK(isValidInviteCode(""));
    CHECK(isValidInviteCode(std::string(32, 'A')));
    CHECK_FALSE(isValidInviteCode(std::string(33, 'A')));
    CHECK_FALSE(isValidInviteCode("café"));
    CHECK_FALSE(isValidInviteCode("ODY_7KQ2"));
}
