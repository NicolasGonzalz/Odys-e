#include "CommandLine.hpp"

#include <doctest/doctest.h>

#include <string_view>
#include <vector>

using odyssee::server::parseCommandLine;

namespace {

auto parse(std::vector<std::string_view> args) { return parseCommandLine(args); }

} // namespace

TEST_CASE("Ligne de commande : valeurs par défaut sans argument") {
    const auto options = parse({});
    REQUIRE(options.has_value());
    CHECK(options->port == odyssee::kDefaultPort);
    CHECK_FALSE(options->verbose);
}

TEST_CASE("Ligne de commande : port et --verbose, dans n'importe quel ordre") {
    for (const auto& args : {std::vector<std::string_view>{"--verbose", "27016"},
                             std::vector<std::string_view>{"27016", "--verbose"}}) {
        const auto options = parse(args);
        REQUIRE(options.has_value());
        CHECK(options->port == 27016);
        CHECK(options->verbose);
    }
}

TEST_CASE("Ligne de commande : arguments invalides refusés") {
    for (const auto& args : {std::vector<std::string_view>{"0"},
                             std::vector<std::string_view>{"65536"},
                             std::vector<std::string_view>{"27016abc"},
                             std::vector<std::string_view>{"-5"},
                             std::vector<std::string_view>{"--inconnu"},
                             std::vector<std::string_view>{"27016", "27017"}}) {
        CHECK_FALSE(parse(args).has_value());
    }
}

TEST_CASE("Ligne de commande : carte à charger") {
    CHECK(parse({}).value().worldPath.empty());
    CHECK(parse({"--world", "cartes/odyssee.world", "27016"}).value().worldPath ==
          "cartes/odyssee.world");
    CHECK_FALSE(parse({"--world"}).has_value());
    CHECK_FALSE(parse({"--world", ""}).has_value());
}

TEST_CASE("Ligne de commande : code d'invitation") {
    CHECK(parse({}).value().inviteCode.empty());
    CHECK(parse({"--invite", "ody-7kq2"}).value().inviteCode == "ODY7KQ2"); // normalisé
    CHECK_FALSE(parse({"--invite"}).has_value());
    CHECK_FALSE(parse({"--invite", ""}).has_value());
    CHECK_FALSE(parse({"--invite", "--"}).has_value());      // vide une fois normalisé
    CHECK_FALSE(parse({"--invite", "pas_bon"}).has_value()); // caractère interdit
}

TEST_CASE("Ligne de commande : simulation réseau") {
    using namespace std::chrono_literals;
    const auto options = parse({"--lag", "120", "27016", "--jitter", "30", "--loss", "5", "--dup",
                                "1"});
    REQUIRE(options.has_value());
    CHECK(options->port == 27016);
    CHECK(options->network.latency == 120ms);
    CHECK(options->network.jitter == 30ms);
    CHECK(options->network.lossPercent == 5.0);
    CHECK(options->network.duplicatePercent == 1.0);

    CHECK(parse({}).value().network.isPerfect());
}

TEST_CASE("Ligne de commande : valeurs de simulation réseau invalides refusées") {
    for (const auto& args : {std::vector<std::string_view>{"--lag"},
                             std::vector<std::string_view>{"--lag", "abc"},
                             std::vector<std::string_view>{"--lag", "-10"},
                             std::vector<std::string_view>{"--lag", "20000"},
                             std::vector<std::string_view>{"--loss", "101"},
                             std::vector<std::string_view>{"--dup", "2.5"},
                             std::vector<std::string_view>{"--jitter", "--verbose"}}) {
        CHECK_FALSE(parse(args).has_value());
    }
}
