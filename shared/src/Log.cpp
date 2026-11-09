#include "odyssee/core/Log.hpp"

#include <atomic>
#include <cstdio>
#include <mutex>

namespace odyssee::log {

namespace {

std::atomic<Level> gLevel{Level::Info};
std::mutex gSinkMutex;
Sink gSink; // vide = sortie par défaut

void defaultSink(const Record& record) {
    const std::string line = format(record);
    std::fprintf(stderr, "%s\n", line.c_str());
    std::fflush(stderr);
}

} // namespace

void setLevel(Level lvl) { gLevel.store(lvl, std::memory_order_relaxed); }

Level level() { return gLevel.load(std::memory_order_relaxed); }

void setSink(Sink sink) {
    const std::scoped_lock lock(gSinkMutex);
    gSink = std::move(sink);
}

std::string_view levelName(Level lvl) {
    switch (lvl) {
    case Level::Trace:
        return "TRACE";
    case Level::Debug:
        return "DEBUG";
    case Level::Info:
        return "INFO ";
    case Level::Warn:
        return "WARN ";
    case Level::Error:
        return "ERROR";
    case Level::Off:
        return "OFF  ";
    }
    return "?????";
}

std::string format(const Record& record) {
    const auto ms = std::chrono::floor<std::chrono::milliseconds>(record.time);
    return std::format("{:%F %T}Z [{}] [{}] {}", ms, levelName(record.level), record.category,
                       record.message);
}

void write(Level lvl, std::string_view category, std::string_view message) {
    if (lvl < level() || lvl == Level::Off) {
        return;
    }
    const Record record{lvl, Clock::now(), category, message};
    const std::scoped_lock lock(gSinkMutex);
    if (gSink) {
        gSink(record);
    } else {
        defaultSink(record);
    }
}

} // namespace odyssee::log
