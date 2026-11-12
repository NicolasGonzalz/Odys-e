#include "ConnectSettings.hpp"

#include <raylib.h>

#include <fstream>
#include <string_view>

namespace odyssee::debugclient {

namespace {

/// Une ligne `clé=valeur` ; les clés inconnues sont ignorées.
void applyLine(ConnectSettings& settings, std::string_view line) {
    const auto equals = line.find('=');
    if (equals == std::string_view::npos) {
        return;
    }
    const std::string_view key = line.substr(0, equals);
    const std::string value(line.substr(equals + 1));
    if (key == "host") {
        settings.host = value;
    } else if (key == "port") {
        settings.port = value;
    } else if (key == "name") {
        settings.name = value;
    } else if (key == "invite") {
        settings.inviteCode = value;
    }
}

} // namespace

ConnectSettings loadSettings(const std::string& path) {
    ConnectSettings settings;
    std::ifstream file(path);
    std::string line;
    while (std::getline(file, line)) {
        applyLine(settings, line);
    }
    return settings;
}

bool saveSettings(const ConnectSettings& settings, const std::string& path) {
    std::ofstream file(path, std::ios::trunc);
    file << "host=" << settings.host << "\n"
         << "port=" << settings.port << "\n"
         << "name=" << settings.name << "\n"
         << "invite=" << settings.inviteCode << "\n";
    return static_cast<bool>(file);
}

std::string settingsPath() {
    return std::string(GetApplicationDirectory()) + "odyssee_client.cfg";
}

} // namespace odyssee::debugclient
