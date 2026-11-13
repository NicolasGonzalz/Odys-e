#pragma once

#include "odyssee/data/Catalog.hpp"
#include "odyssee/net/GameEvents.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

namespace odyssee::debugclient {

/// Menu d'artisanat du client de debug (T047) : C l'ouvre, flèches haut/bas pour choisir,
/// Entrée pour fabriquer. Le serveur décide ; le menu montre sa dernière réponse.
class CraftMenu {
  public:
    void toggle() { _open = !_open; }
    [[nodiscard]] bool isOpen() const { return _open; }

    /// Touches du menu ouvert : la recette à demander si Entrée vient d'être pressée.
    [[nodiscard]] std::optional<std::uint16_t> handleKeys(const data::Catalog& catalog);

    /// Une réponse du serveur, mise en clair pour le joueur.
    void onStatus(const net::CraftStatus& status, const data::Catalog& catalog);

    /// Dernière réponse du serveur (vide au départ), affichée même menu fermé.
    [[nodiscard]] const std::string& lastMessage() const { return _message; }

    /// Dessine la liste des recettes (si ouvert).
    void draw(const data::Catalog& catalog) const;

  private:
    bool _open = false;
    std::size_t _cursor = 0;
    std::string _message;
};

} // namespace odyssee::debugclient
