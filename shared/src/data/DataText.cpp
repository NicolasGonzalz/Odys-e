#include "odyssee/data/DataText.hpp"

#include <string>
#include <utility>

namespace odyssee::data {

namespace {

constexpr std::string_view kUtf8Bom = "\xEF\xBB\xBF";
constexpr std::string_view kBlanks = " \t";

std::string_view trim(std::string_view text) {
    const std::size_t first = text.find_first_not_of(kBlanks);
    if (first == std::string_view::npos) {
        return {};
    }
    const std::size_t last = text.find_last_not_of(kBlanks);
    return text.substr(first, last - first + 1);
}

/// Octet de contrôle (nul compris), hors tabulation : interdit partout.
bool hasControlByte(std::string_view line) {
    for (const char c : line) {
        const auto byte = static_cast<unsigned char>(c);
        if ((byte < 0x20 && c != '\t') || byte == 0x7F) {
            return true;
        }
    }
    return false;
}

/// Découpe ligne par ligne et range chaque ligne dans le bon bloc.
class Parser {
  public:
    explicit Parser(std::string_view fileName) : _fileName(fileName) {}

    DataBlocks run(std::string_view text) {
        if (text.starts_with(kUtf8Bom)) {
            text.remove_prefix(kUtf8Bom.size());
        }
        std::size_t lineNumber = 0;
        while (!text.empty() && !_result.error) {
            const std::size_t end = text.find('\n');
            std::string_view line = text.substr(0, end);
            text.remove_prefix(end == std::string_view::npos ? text.size() : end + 1);
            if (line.ends_with('\r')) {
                line.remove_suffix(1);
            }
            readLine(line, ++lineNumber);
        }
        return std::move(_result);
    }

  private:
    void readLine(std::string_view line, std::size_t lineNumber) {
        if (line.size() > kMaxLineLength) {
            fail(lineNumber, "ligne trop longue (" + std::to_string(line.size()) +
                                 " octets, maximum " + std::to_string(kMaxLineLength) + ")");
            return;
        }
        if (hasControlByte(line)) {
            fail(lineNumber, "caractère de contrôle interdit (octet nul, etc.)");
            return;
        }

        const std::string_view content = trim(line);
        if (content.empty() || content.starts_with('#')) {
            return;
        }
        if (content.starts_with('[')) {
            readHeader(content, lineNumber);
        } else {
            readField(content, lineNumber);
        }
    }

    /// « [kind id] » : ouvre un nouveau bloc.
    void readHeader(std::string_view content, std::size_t lineNumber) {
        if (!content.ends_with(']')) {
            fail(lineNumber, "en-tête de bloc mal fermé : « ] » attendu en fin de ligne");
            return;
        }
        const std::string_view inner = trim(content.substr(1, content.size() - 2));
        const std::size_t space = inner.find_first_of(kBlanks);
        if (space == std::string_view::npos) {
            fail(lineNumber, "en-tête de bloc incomplet : « [type identifiant] » attendu");
            return;
        }
        const std::string_view kind = inner.substr(0, space);
        const std::string_view id = trim(inner.substr(space));
        if (id.find_first_of(kBlanks) != std::string_view::npos) {
            fail(lineNumber, "en-tête de bloc : un seul identifiant attendu après le type");
            return;
        }
        if (_result.blocks.size() >= kMaxBlocks) {
            fail(lineNumber, "trop de blocs (maximum " + std::to_string(kMaxBlocks) + ")");
            return;
        }
        _result.blocks.push_back({kind, id, lineNumber, {}});
    }

    /// « clé = valeur » : ajoutée au bloc courant.
    void readField(std::string_view content, std::size_t lineNumber) {
        const std::size_t equal = content.find('=');
        if (equal == std::string_view::npos) {
            fail(lineNumber,
                 "ligne incomprise : « clé = valeur » ou « [type identifiant] » attendu");
            return;
        }
        const std::string_view key = trim(content.substr(0, equal));
        const std::string_view value = trim(content.substr(equal + 1));
        if (key.empty()) {
            fail(lineNumber, "clé manquante avant « = »");
            return;
        }
        if (value.empty()) {
            fail(lineNumber, "clé « " + std::string(key) + " » sans valeur");
            return;
        }
        addField({key, value, lineNumber});
    }

    void addField(const DataField& field) {
        if (_result.blocks.empty()) {
            fail(field.line, "ligne hors bloc : commencer par un en-tête « [type identifiant] »");
            return;
        }
        DataBlock& block = _result.blocks.back();
        if (block.fields.size() >= kMaxFieldsPerBlock) {
            fail(field.line, "trop de lignes dans ce bloc (maximum " +
                                 std::to_string(kMaxFieldsPerBlock) + ")");
            return;
        }
        block.fields.push_back(field);
    }

    void fail(std::size_t lineNumber, std::string message) {
        _result.error = DataError{std::string(_fileName), lineNumber, std::move(message)};
    }

    std::string_view _fileName;
    DataBlocks _result;
};

} // namespace

DataBlocks parseDataText(std::string_view text, std::string_view fileName) {
    return Parser(fileName).run(text);
}

} // namespace odyssee::data
