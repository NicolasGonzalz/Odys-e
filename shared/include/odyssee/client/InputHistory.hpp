#pragma once

#include "odyssee/core/Config.hpp"
#include "odyssee/net/GameMessages.hpp"

#include <cstddef>
#include <deque>
#include <vector>

namespace odyssee::client {

/// Les derniers inputs du joueur local, numérotés par tick du client (consécutifs).
///
/// - Renvoyés en lot à chaque paquet : un paquet perdu ne fait perdre aucun input, le suivant
///   le contient encore (redondance plutôt que renvoi).
/// - Gardés quelques secondes pour la réconciliation (T032) : rejouer ceux que le serveur n'a
///   pas encore traités.
class InputHistory {
  public:
    /// Inputs gardés au plus : 3,2 s à 20 ticks/s, bien plus qu'un aller-retour réseau.
    static constexpr std::size_t kCapacity = 64;

    /// Ajoute l'input du tick `tick`. Les ticks se suivent ; un tick qui ne suit pas le
    /// précédent repart d'un historique vide (il n'y a pas de trou dans un lot).
    void record(net::Tick tick, const net::InputCommand& command);

    /// Les `count` derniers inputs, prêts à envoyer (lot vide si aucun input).
    [[nodiscard]] net::InputBatch recentBatch(std::size_t count = kMaxInputsPerBatch) const;

    [[nodiscard]] bool empty() const { return _commands.empty(); }
    [[nodiscard]] std::size_t size() const { return _commands.size(); }

    /// Tick du plus récent input (0 si aucun).
    [[nodiscard]] net::Tick lastTick() const;

    /// Vrai si l'historique contient tous les inputs joués après `tick` (rejouables).
    [[nodiscard]] bool covers(net::Tick tick) const;

    /// Les inputs joués après `tick`, dans l'ordre (vide si covers(tick) est faux).
    [[nodiscard]] std::vector<net::InputCommand> after(net::Tick tick) const;

  private:
    net::Tick _firstTick = 0; ///< tick du plus ancien input gardé
    std::deque<net::InputCommand> _commands;
};

} // namespace odyssee::client
