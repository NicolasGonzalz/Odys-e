#include "WinsockRuntime.hpp"

#include "WinsockHeaders.hpp"
#include "odyssee/core/Log.hpp"

namespace odyssee::net::win32 {

namespace {

constexpr log::Logger kLog{"net"};

/// Démarre Winsock à la construction et le libère à la destruction (fin du programme).
class WinsockSession {
  public:
    WinsockSession() {
        WSADATA data{};
        const int result = ::WSAStartup(MAKEWORD(2, 2), &data);
        _started = result == 0;
        if (!_started) {
            kLog.error("WSAStartup a échoué (code {})", result);
        }
    }
    ~WinsockSession() {
        if (_started) {
            ::WSACleanup();
        }
    }
    WinsockSession(const WinsockSession&) = delete;
    WinsockSession& operator=(const WinsockSession&) = delete;

    [[nodiscard]] bool started() const { return _started; }

  private:
    bool _started = false;
};

} // namespace

bool ensureWinsockStarted() {
    // Variable statique locale : construite une seule fois, au premier appel (thread-safe).
    static const WinsockSession session;
    return session.started();
}

} // namespace odyssee::net::win32
