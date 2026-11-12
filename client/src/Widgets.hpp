#pragma once

#include <raylib.h>

#include <cstddef>
#include <string>

namespace odyssee::debugclient {

/// Éléments d'interface minimaux du client de debug (raylib dessine, on gère le reste).

/// Un champ de saisie d'une ligne.
struct TextField {
    const char* label = "";
    std::string* text = nullptr; ///< le texte édité (appartient à l'appelant)
    std::size_t maxBytes = 64;
    bool digitsOnly = false;
};

/// Ajoute les caractères tapés depuis l'image précédente au champ, efface avec Retour arrière.
void typeInto(const TextField& field);

/// Dessine le champ (libellé au-dessus) ; `focused` : il reçoit la saisie.
void drawField(const TextField& field, Rectangle box, bool focused);

/// Dessine un bouton ; vrai s'il vient d'être cliqué.
bool button(Rectangle box, const char* label);

/// Texte centré horizontalement sur l'écran.
void drawCentered(const char* text, int y, int size, Color color);

} // namespace odyssee::debugclient
