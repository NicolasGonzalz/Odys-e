// Déterminisme de la simulation (T030) : même suite d'inputs = même état, partout.
//
// Le test « valeur de référence » joue une longue partie scriptée et compare l'empreinte de tous
// les états traversés à une valeur figée. La CI le lance sous Linux (GCC, sanitizers) et sous
// Windows (MSVC, le compilateur du client Unreal) : s'il passe des deux côtés, la simulation donne
// le même résultat au bit près sur le serveur et sur le client.
//
// Si un réglage ou une règle de mouvement change volontairement, la valeur change : la relever
// dans le message d'échec et la mettre à jour ici, dans le même commit que le changement.
#include "odyssee/net/Quantize.hpp"
#include "odyssee/sim/FixedMath.hpp"
#include "odyssee/sim/Movement.hpp"
#include "odyssee/sim/Replication.hpp"

#include <doctest/doctest.h>

#include <cstdint>
#include <vector>

using namespace odyssee;
using namespace odyssee::sim;

namespace {

constexpr std::int32_t kMeter = 256;

/// Générateur pseudo-aléatoire entièrement spécifié (LCG 64 bits de Knuth) : mêmes tirages sur
/// toutes les plateformes, contrairement aux distributions de la bibliothèque standard.
class Script {
  public:
    explicit Script(std::uint64_t seed) : _state(seed) {}

    std::uint32_t next() {
        _state = _state * 6364136223846793005ULL + 1442695040888963407ULL;
        return static_cast<std::uint32_t>(_state >> 33);
    }
    bool chance(std::uint32_t percent) { return next() % 100 < percent; }

  private:
    std::uint64_t _state;
};

/// Collines (deux ondulations de quelques mètres) et une falaise de 3 m : de quoi passer par tous
/// les cas (pentes, marches, murs, chutes). Entiers seulement, comme le terrain réel devra l'être.
class Hills final : public Terrain {
  public:
    [[nodiscard]] std::int32_t groundHeight(std::int32_t x, std::int32_t y) const override {
        const std::int32_t waveX = sinQ15(static_cast<std::uint16_t>(x / 3)) * 4 * kMeter;
        const std::int32_t waveY = cosQ15(static_cast<std::uint16_t>(y / 5)) * 3 * kMeter;
        const std::int32_t cliff = (x / (40 * kMeter)) % 3 == 1 ? 3 * kMeter : 0;
        return waveX / kQ15One + waveY / kQ15One + cliff;
    }
};

/// Un input tiré au hasard, gardé quelques ticks (comme un joueur qui tient une direction).
net::InputCommand randomInput(Script& script) {
    constexpr std::int8_t axes[] = {-127, -64, 0, 0, 64, 127};
    net::InputCommand command;
    command.moveForward = axes[script.next() % 6];
    command.moveRight = axes[script.next() % 6];
    command.yaw = static_cast<std::uint16_t>(script.next());
    command.pitch = static_cast<std::int16_t>(static_cast<std::int32_t>(script.next() % 65535) -
                                              net::kMaxPitchUnits);
    unsigned buttons = 0;
    buttons |= script.chance(85) ? static_cast<unsigned>(net::InputButton::Sprint) : 0U;
    buttons |= script.chance(15) ? static_cast<unsigned>(net::InputButton::Jump) : 0U;
    buttons |= script.chance(10) ? static_cast<unsigned>(net::InputButton::Crouch) : 0U;
    command.buttons = static_cast<std::uint16_t>(buttons);
    return command;
}

std::vector<net::InputCommand> scriptedInputs(std::uint64_t seed, int ticks) {
    Script script(seed);
    std::vector<net::InputCommand> inputs;
    net::InputCommand current;
    for (int i = 0; i < ticks; ++i) {
        if (i % 15 == 0) {
            current = randomInput(script);
        }
        inputs.push_back(current);
    }
    return inputs;
}

/// Empreinte FNV-1a 64 bits de chaque champ de l'état : une seule différence, à n'importe quel
/// tick, la change.
class Fingerprint {
  public:
    void add(std::int64_t value) {
        for (int i = 0; i < 8; ++i) {
            _hash ^= static_cast<std::uint64_t>(value >> (8 * i)) & 0xFF;
            _hash *= 0x100000001B3ULL;
        }
    }
    void add(const PlayerState& s) {
        for (const auto v : s.position) {
            add(v);
        }
        for (const auto v : s.velocity) {
            add(v);
        }
        add(s.yaw);
        add(s.pitch);
        add(s.stamina);
        add(s.flags);
    }
    [[nodiscard]] std::uint64_t value() const { return _hash; }

  private:
    std::uint64_t _hash = 0xCBF29CE484222325ULL;
};

PlayerState spawnOn(const Terrain& terrain) {
    PlayerState state;
    state.position = {500 * kMeter, 700 * kMeter, 0};
    state.position[2] = terrain.groundHeight(state.position[0], state.position[1]);
    return state;
}

} // namespace

TEST_CASE("Déterminisme : une longue partie scriptée donne la valeur de référence") {
    const Hills hills;
    const auto inputs = scriptedInputs(2026, 2000);

    PlayerState state = spawnOn(hills);
    Fingerprint fingerprint;
    int airborne = 0;
    int exhausted = 0;
    int blocked = 0;
    for (const auto& input : inputs) {
        const PlayerState before = state;
        state = step(state, input, hills);
        fingerprint.add(state);
        airborne += state.has(kGrounded) ? 0 : 1;
        exhausted += state.has(kExhausted) ? 1 : 0;
        const bool wanted = input.moveForward != 0 || input.moveRight != 0;
        blocked += wanted && before.has(kGrounded) && state.position[0] == before.position[0] &&
                           state.position[1] == before.position[1]
                       ? 1
                       : 0;
    }

    // La partie passe bien par tous les cas : sinon la valeur de référence ne prouverait rien.
    CHECK(airborne > 100);
    CHECK(exhausted > 10);
    CHECK(blocked > 0);

    // Valeur obtenue une première fois (MSVC), puis figée (voir l'en-tête du fichier).
    constexpr std::uint64_t kReference = 15280125415309132226ULL;
    MESSAGE("empreinte : " << fingerprint.value());
    CHECK(fingerprint.value() == kReference);
}

TEST_CASE("Déterminisme : rejouer la même suite donne exactement les mêmes états") {
    const Hills hills;
    const auto inputs = scriptedInputs(7, 600);

    PlayerState a = spawnOn(hills);
    PlayerState b = spawnOn(hills);
    for (const auto& input : inputs) {
        a = step(a, input, hills);
        b = step(b, input, hills);
        REQUIRE(a == b);
    }
}

TEST_CASE("Déterminisme : repartir d'un snapshot, puis rejouer, retombe sur l'état du serveur") {
    // Ce que fera la réconciliation (T032) : le client reconstitue l'état envoyé par le serveur
    // à un tick, rejoue ses inputs suivants, et doit retrouver exactement l'état du serveur.
    const Hills hills;
    const auto inputs = scriptedInputs(99, 900);

    PlayerState server = spawnOn(hills);
    for (std::size_t tick = 0; tick < inputs.size(); ++tick) {
        server = step(server, inputs[tick], hills);
        if (tick % 97 != 0 || tick + 30 > inputs.size()) {
            continue;
        }
        PlayerState client = restore(toEntity(0, server), toOwnMotion(server));
        PlayerState truth = server;
        for (std::size_t replay = tick + 1; replay < tick + 30; ++replay) {
            client = step(client, inputs[replay], hills);
            truth = step(truth, inputs[replay], hills);
        }
        CAPTURE(tick);
        CHECK(client == truth);
    }
}

TEST_CASE("Déterminisme : la table trigonométrique est celle attendue") {
    // Si quelqu'un régénère ou modifie la table, les états changent partout : on le voit ici.
    std::int64_t sum = 0;
    for (std::uint32_t yaw = 0; yaw < 65536; yaw += 13) {
        sum += sinQ15(static_cast<std::uint16_t>(yaw)) * static_cast<std::int64_t>(yaw % 101);
        sum += cosQ15(static_cast<std::uint16_t>(yaw));
    }
    constexpr std::int64_t kReferenceSum = 38034;
    MESSAGE("somme : " << sum);
    CHECK(sum == kReferenceSum);
}
