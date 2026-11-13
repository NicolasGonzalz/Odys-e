#pragma once

#include <cstddef>
#include <string>

namespace odyssee::data {

/// Erreur dans un fichier de données (docs/DATA_FORMAT.md) : où, et pourquoi, en français.
/// Les fichiers sont édités à la main : le message doit suffire à corriger sans lire le code.
struct DataError {
    std::string file;     ///< nom ou chemin du fichier fautif
    std::size_t line = 0; ///< numéro de ligne (à partir de 1), 0 si le fichier entier est en cause
    std::string message;

    /// « items.txt:12 : message » (ou « items.txt : message » sans ligne).
    [[nodiscard]] std::string describe() const {
        if (line == 0) {
            return file + " : " + message;
        }
        return file + ":" + std::to_string(line) + " : " + message;
    }
};

} // namespace odyssee::data
