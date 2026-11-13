#include "DataValues.hpp"

#include <algorithm>
#include <charconv>
#include <utility>

namespace odyssee::data {

namespace {

bool contains(std::span<const std::string_view> list, std::string_view key) {
    return std::find(list.begin(), list.end(), key) != list.end();
}

bool isKeyStart(char c) {
    return c >= 'a' && c <= 'z';
}

bool isKeyChar(char c) {
    return isKeyStart(c) || (c >= '0' && c <= '9') || c == '_';
}

} // namespace

std::string quoted(std::string_view text) {
    return "« " + std::string(text) + " »";
}

DataError makeError(std::string_view file, std::size_t line, std::string message) {
    return DataError{std::string(file), line, std::move(message)};
}

std::optional<std::uint32_t> parseNumber(std::string_view text, std::uint32_t min,
                                         std::uint32_t max) {
    std::uint32_t value = 0;
    const char* end = text.data() + text.size();
    const auto [stop, error] = std::from_chars(text.data(), end, value);
    if (error != std::errc{} || stop != end || value < min || value > max) {
        return std::nullopt;
    }
    return value;
}

bool isValidKey(std::string_view text) {
    if (text.empty() || text.size() > kMaxKeyLength || !isKeyStart(text.front())) {
        return false;
    }
    return std::all_of(text.begin(), text.end(), isKeyChar);
}

bool isValidModelPath(std::string_view text) {
    if (text.empty() || text.size() > kMaxModelPathLength) {
        return false;
    }
    while (true) {
        const std::size_t slash = text.find('/');
        if (!isValidKey(text.substr(0, slash))) {
            return false;
        }
        if (slash == std::string_view::npos) {
            return true;
        }
        text.remove_prefix(slash + 1);
    }
}

std::optional<DataError> checkBlockKeys(const DataBlock& block, const BlockKeys& keys,
                                        std::string_view file) {
    for (const DataField& field : block.fields) {
        if (!contains(keys.allowed, field.key)) {
            return makeError(file, field.line, "clé inconnue " + quoted(field.key));
        }
        if (!contains(keys.repeatable, field.key) && findField(block, field.key) != &field) {
            return makeError(file, field.line, "clé " + quoted(field.key) + " répétée");
        }
    }
    for (const std::string_view key : keys.required) {
        if (findField(block, key) == nullptr) {
            return makeError(file, block.line, "clé " + quoted(key) + " manquante dans ce bloc");
        }
    }
    return std::nullopt;
}

const DataField* findField(const DataBlock& block, std::string_view key) {
    for (const DataField& field : block.fields) {
        if (field.key == key) {
            return &field;
        }
    }
    return nullptr;
}

} // namespace odyssee::data
