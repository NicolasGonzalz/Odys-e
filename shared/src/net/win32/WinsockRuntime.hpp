// Interne à shared/ : Winsock doit être initialisé (WSAStartup) avant tout socket,
// et libéré (WSACleanup) à la fin du programme.
#pragma once

namespace odyssee::net::win32 {

/// Initialise Winsock au premier appel (sans effet ensuite). Renvoie false en cas d'échec.
/// La libération est automatique à la fin du programme.
[[nodiscard]] bool ensureWinsockStarted();

} // namespace odyssee::net::win32
