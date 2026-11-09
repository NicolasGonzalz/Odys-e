#pragma once

#include <chrono>
#include <cstdint>
#include <format>
#include <functional>
#include <string>
#include <string_view>
#include <utility>

namespace odyssee::log {

enum class Level : std::uint8_t { Trace, Debug, Info, Warn, Error, Off };

using Clock = std::chrono::system_clock;

/// Une entrée de log, déjà formatée côté message.
struct Record {
    Level level;
    Clock::time_point time;
    std::string_view category;
    std::string_view message;
};

/// Destination des logs. Par défaut : stderr. Le plugin Unreal la remplacera par UE_LOG,
/// les tests par une capture en mémoire.
using Sink = std::function<void(const Record&)>;

/// Niveau minimal émis (global). Les messages en dessous ne sont même pas formatés.
void setLevel(Level level);
[[nodiscard]] Level level();

/// Remplace la destination des logs ; un Sink vide restaure la sortie par défaut (stderr).
void setSink(Sink sink);

/// Mise en forme standard : "2026-11-16 10:42:01.123Z [INFO ] [server] message".
[[nodiscard]] std::string format(const Record& record);

[[nodiscard]] std::string_view levelName(Level level);

/// Envoie un message déjà formaté à la destination courante (thread-safe).
void write(Level level, std::string_view category, std::string_view message);

/// Point d'entrée d'un module : `inline constexpr Logger kLog{"server"}; kLog.info("tick {}", n);`
class Logger {
  public:
    constexpr explicit Logger(std::string_view category) : _category(category) {}

    template <typename... Args>
    void log(Level lvl, std::format_string<Args...> fmt, Args&&... args) const {
        if (lvl < level()) {
            return;
        }
        write(lvl, _category, std::format(fmt, std::forward<Args>(args)...));
    }

    // clang-format off
    template <typename... Args> void trace(std::format_string<Args...> f, Args&&... a) const { log(Level::Trace, f, std::forward<Args>(a)...); }
    template <typename... Args> void debug(std::format_string<Args...> f, Args&&... a) const { log(Level::Debug, f, std::forward<Args>(a)...); }
    template <typename... Args> void info(std::format_string<Args...> f, Args&&... a) const { log(Level::Info, f, std::forward<Args>(a)...); }
    template <typename... Args> void warn(std::format_string<Args...> f, Args&&... a) const { log(Level::Warn, f, std::forward<Args>(a)...); }
    template <typename... Args> void error(std::format_string<Args...> f, Args&&... a) const { log(Level::Error, f, std::forward<Args>(a)...); }
    // clang-format on

    [[nodiscard]] constexpr std::string_view category() const { return _category; }

  private:
    std::string_view _category;
};

} // namespace odyssee::log
