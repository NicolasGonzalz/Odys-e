#pragma once

#include "odyssee/net/Sequence.hpp"

#include <cstdint>
#include <vector>

namespace odyssee::net {

/// Décode les champs `ack` / `ackBits` reçus de l'autre côté : la liste des numéros
/// de **nos** paquets qu'il confirme avoir reçus (du plus récent au plus ancien, 33 au maximum).
[[nodiscard]] std::vector<Sequence> acknowledgedSequences(Sequence ack, std::uint32_t ackBits);

} // namespace odyssee::net
