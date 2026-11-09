#include "odyssee/core/Log.hpp"

#include <doctest/doctest.h>

#include <string>
#include <vector>

namespace olog = odyssee::log;

namespace {

struct Captured {
    olog::Level level;
    std::string category;
    std::string message;
};

/// Capture les logs pendant la durée de vie de l'objet, puis restaure l'état global.
class Capture {
  public:
    explicit Capture(olog::Level minLevel = olog::Level::Trace) : _previousLevel(olog::level()) {
        olog::setLevel(minLevel);
        olog::setSink([this](const olog::Record& r) {
            records.push_back({r.level, std::string(r.category), std::string(r.message)});
        });
    }
    ~Capture() {
        olog::setSink({});
        olog::setLevel(_previousLevel);
    }
    Capture(const Capture&) = delete;
    Capture& operator=(const Capture&) = delete;

    std::vector<Captured> records;

  private:
    olog::Level _previousLevel;
};

constexpr olog::Logger kTestLog{"test"};

} // namespace

TEST_CASE("Log : formatage des arguments et catégorie") {
    Capture cap;
    kTestLog.info("tick {} sur {}", 42, "serveur");
    REQUIRE(cap.records.size() == 1);
    CHECK(cap.records[0].level == olog::Level::Info);
    CHECK(cap.records[0].category == "test");
    CHECK(cap.records[0].message == "tick 42 sur serveur");
}

TEST_CASE("Log : les messages sous le niveau minimal sont ignorés") {
    Capture cap(olog::Level::Warn);
    kTestLog.debug("invisible");
    kTestLog.info("invisible aussi");
    kTestLog.warn("visible");
    kTestLog.error("visible aussi");
    REQUIRE(cap.records.size() == 2);
    CHECK(cap.records[0].level == olog::Level::Warn);
    CHECK(cap.records[1].level == olog::Level::Error);
}

TEST_CASE("Log : le niveau Off coupe tout") {
    Capture cap(olog::Level::Off);
    kTestLog.error("rien");
    CHECK(cap.records.empty());
}

TEST_CASE("Log : mise en forme standard avec horodatage UTC à la milliseconde") {
    using namespace std::chrono;
    const auto day = sys_days{year{2026} / November / 16};
    const olog::Record record{olog::Level::Warn, day + 10h + 42min + 1s + 123ms, "net",
                             "paquet rejeté"};
    CHECK(olog::format(record) == "2026-11-16 10:42:01.123Z [WARN ] [net] paquet rejeté");
}
