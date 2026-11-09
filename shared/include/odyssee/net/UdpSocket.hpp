#pragma once

#include "odyssee/net/Address.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace odyssee::net {

/// Un datagramme reçu : qui l'a envoyé, et combien d'octets ont été écrits dans le tampon.
struct Received {
    Address from;
    std::size_t size = 0;
};

/// Socket UDP IPv4 **non bloquant**.
///
/// - RAII : le socket est fermé à la destruction. Déplaçable, non copiable (un seul propriétaire).
/// - Aucune exception : les erreurs sont signalées par std::nullopt / false
///   (règle de shared/, ADR 0003).
/// - Implémentation par plateforme : net/posix/ (Linux/macOS), net/win32/ (Windows).
class UdpSocket {
  public:
    /// Ouvre un socket attaché à `port` sur toutes les interfaces.
    /// `port = 0` : le système choisit un port libre (clients, tests).
    /// Renvoie std::nullopt en cas d'échec (port déjà utilisé, droits insuffisants…).
    [[nodiscard]] static std::optional<UdpSocket> open(std::uint16_t port = 0);

    ~UdpSocket();
    UdpSocket(UdpSocket&& other) noexcept;
    UdpSocket& operator=(UdpSocket&& other) noexcept;
    UdpSocket(const UdpSocket&) = delete;
    UdpSocket& operator=(const UdpSocket&) = delete;

    /// Envoie un datagramme. Renvoie false si l'envoi échoue ou si `data` dépasse kMaxPacketSize.
    /// Un `true` ne garantit pas l'arrivée : c'est de l'UDP.
    [[nodiscard]] bool send(const Address& to, std::span<const std::byte> data);

    /// Lit le prochain datagramme dans `buffer`, sans jamais attendre.
    /// Renvoie std::nullopt quand il n'y a plus rien à lire. Les datagrammes trop gros pour
    /// `buffer` sont ignorés (et journalisés) ; la lecture passe alors au suivant.
    [[nodiscard]] std::optional<Received> receive(std::span<std::byte> buffer);

    /// Port local effectivement utilisé (utile après open(0)).
    [[nodiscard]] std::uint16_t localPort() const { return _port; }

  private:
    // Identifiant système du socket : un int sous POSIX, un SOCKET (entier non signé) sous Windows.
    using Handle = std::intptr_t;
    static constexpr Handle kInvalidHandle = -1;

    UdpSocket(Handle handle, std::uint16_t port) : _handle(handle), _port(port) {}
    void close();

    Handle _handle = kInvalidHandle;
    std::uint16_t _port = 0;
};

} // namespace odyssee::net
