#include "ConnectScreen.hpp"

#include "odyssee/core/Config.hpp"
#include "odyssee/net/InviteCode.hpp"
#include "odyssee/net/PlayerName.hpp"

#include <raylib.h>

#include <charconv>
#include <utility>

namespace odyssee::debugclient {

namespace {

constexpr float kWidth = 460.0f;
constexpr float kFieldHeight = 40.0f;
constexpr float kRowSpacing = 74.0f;
constexpr float kTop = 150.0f;

/// Port tapé : vide = port par défaut.
std::optional<std::uint16_t> portFrom(const std::string& text) {
    if (text.empty()) {
        return kDefaultPort;
    }
    unsigned value = 0;
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (error != std::errc{} || end != text.data() + text.size() || value == 0 || value > 65535) {
        return std::nullopt;
    }
    return static_cast<std::uint16_t>(value);
}

float left() {
    return (static_cast<float>(GetScreenWidth()) - kWidth) / 2.0f;
}

} // namespace

ConnectScreen::ConnectScreen(ConnectSettings settings, std::string message)
    : _settings(std::move(settings)), _message(std::move(message)) {}

std::optional<ConnectTarget> ConnectScreen::run() {
    EnableCursor();
    while (!WindowShouldClose()) {
        handleFocus();
        typeInto(fields()[_focus]);

        bool connectClicked = false;
        bool quitClicked = false;
        draw(connectClicked, quitClicked);
        if (quitClicked) {
            return std::nullopt;
        }
        if (connectClicked || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) {
            if (auto target = validate()) {
                return target;
            }
        }
    }
    return std::nullopt;
}

std::array<TextField, ConnectScreen::kFieldCount> ConnectScreen::fields() {
    return {TextField{"Adresse du serveur (IPv4)", &_settings.host, 15, false},
            TextField{"Port (vide : 27015)", &_settings.port, 5, true},
            TextField{"Pseudo", &_settings.name, kMaxPlayerNameLength, false},
            TextField{"Code d'invitation (si le serveur en demande un)", &_settings.inviteCode, 48,
                      false}};
}

std::array<Rectangle, ConnectScreen::kFieldCount> ConnectScreen::fieldBoxes() const {
    std::array<Rectangle, kFieldCount> boxes{};
    for (std::size_t i = 0; i < kFieldCount; ++i) {
        boxes[i] = {left(), kTop + kRowSpacing * static_cast<float>(i), kWidth, kFieldHeight};
    }
    return boxes;
}

void ConnectScreen::handleFocus() {
    if (IsKeyPressed(KEY_TAB)) {
        const bool back = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
        _focus = (_focus + (back ? kFieldCount - 1 : 1)) % kFieldCount;
    }
    if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        return;
    }
    const auto boxes = fieldBoxes();
    for (std::size_t i = 0; i < kFieldCount; ++i) {
        if (CheckCollisionPointRec(GetMousePosition(), boxes[i])) {
            _focus = i;
        }
    }
}

std::optional<ConnectTarget> ConnectScreen::validate() {
    const auto port = portFrom(_settings.port);
    const auto server = port ? net::Address::parse(_settings.host, *port) : std::nullopt;
    if (!server) {
        _message = "Adresse ou port invalide (exemple : 82.64.10.20 et 27015)";
    } else if (!net::isValidPlayerName(_settings.name)) {
        _message = "Pseudo invalide";
    } else if (!net::isValidInviteCode(_settings.inviteCode)) {
        _message = "Code d'invitation invalide : lettres et chiffres seulement";
    } else {
        return ConnectTarget{_settings, *server};
    }
    return std::nullopt;
}

void ConnectScreen::draw(bool& connectClicked, bool& quitClicked) {
    BeginDrawing();
    ClearBackground(Color{22, 26, 31, 255});
    drawCentered("Odyssée - client de debug", 60, 34, RAYWHITE);

    const auto boxes = fieldBoxes();
    const auto all = fields();
    for (std::size_t i = 0; i < kFieldCount; ++i) {
        drawField(all[i], boxes[i], i == _focus);
    }
    const float y = kTop + kRowSpacing * static_cast<float>(kFieldCount);
    DrawText(_message.c_str(), static_cast<int>(left()), static_cast<int>(y), 18,
             Color{240, 150, 70, 255});
    connectClicked = button({left(), y + 36, kWidth * 0.6f, 44}, "Se connecter");
    quitClicked = button({left() + kWidth * 0.65f, y + 36, kWidth * 0.35f, 44}, "Quitter");
    drawCentered("Tab : champ suivant - Entrée : se connecter", GetScreenHeight() - 40, 18, GRAY);
    EndDrawing();
}

} // namespace odyssee::debugclient
