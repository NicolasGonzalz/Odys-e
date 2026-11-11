#pragma once

#include "odyssee/net/GameMessages.hpp"

#include <cstddef>
#include <deque>
#include <optional>

namespace odyssee::server {

/// Inputs d'un joueur en attente, appliqués un par tick serveur dans l'ordre de leurs ticks.
///
/// Chaque paquet du client répète ses derniers inputs (voir InputBatch) : la file ne garde que
/// ceux qu'elle n'a pas encore vus. Un input perdu malgré la redondance est simplement sauté.
class InputQueue {
  public:
    /// Retard maximal : au-delà de 8 inputs en attente (400 ms), les plus anciens sont sautés
    /// pour que le joueur ne soit pas joué de plus en plus en retard.
    static constexpr std::size_t kMaxBacklog = 8;

    struct Entry {
        net::Tick tick = 0;
        net::InputCommand command;
        friend bool operator==(const Entry&, const Entry&) = default;
    };

    /// Ajoute les inputs du lot plus récents que tous ceux déjà reçus.
    void push(const net::InputBatch& batch);

    /// Input à appliquer à ce tick (le plus ancien en attente), s'il y en a un.
    [[nodiscard]] std::optional<Entry> pop();

    [[nodiscard]] std::size_t size() const { return _pending.size(); }

  private:
    std::deque<Entry> _pending;
    std::optional<net::Tick> _newestReceived;
};

} // namespace odyssee::server
