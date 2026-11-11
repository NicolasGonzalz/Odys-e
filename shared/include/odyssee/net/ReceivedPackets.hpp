#pragma once

#include "odyssee/net/Sequence.hpp"

#include <cstdint>

namespace odyssee::net {

/// Ce qu'on peut dire d'un paquet entrant, d'après son numéro.
enum class Arrival : std::uint8_t {
    New,       ///< jamais vu : à traiter
    Duplicate, ///< déjà reçu : à ignorer
    TooOld,    ///< plus de 32 paquets de retard : impossible de savoir, à ignorer
    TooFarAhead, ///< saut de numéro invraisemblable : paquet forgé ou corrompu, à ignorer
};

/// Mémorise les numéros des paquets reçus de l'autre côté, pour :
/// - détecter les doublons ;
/// - remplir les champs `ack` / `ackBits` de nos prochains en-têtes.
class ReceivedPackets {
  public:
    /// Saut maximal accepté entre le paquet le plus récent et un nouveau. Bien plus que ce qu'une
    /// connexion vivante peut perdre d'affilée (10 s de silence la ferment), mais assez peu pour
    /// qu'un seul paquet ne puisse pas décaler la fenêtre d'un demi-tour et faire passer tous
    /// les vrais paquets suivants pour « trop vieux ».
    static constexpr std::uint16_t kMaxJump = 1024;

    /// Enregistre l'arrivée du paquet `sequence` et dit s'il est nouveau. Rien n'est enregistré
    /// pour un paquet refusé.
    Arrival record(Sequence sequence);

    /// Dernier paquet reçu (le plus récent).
    [[nodiscard]] Sequence ack() const { return _latest; }

    /// Bit i = paquet (ack - 1 - i) reçu.
    [[nodiscard]] std::uint32_t ackBits() const { return _bits; }

  private:
    Arrival recordNewer(Sequence sequence);
    Arrival recordOlder(Sequence sequence);

    bool _hasReceived = false;
    // Tant que rien n'est reçu, ack() vaut 65535 : un numéro que l'autre côté n'a pas encore
    // utilisé (il commence à 0). Avec 0, on « confirmerait » à tort son tout premier paquet.
    Sequence _latest = 0xFFFF;
    std::uint32_t _bits = 0;
};

} // namespace odyssee::net
