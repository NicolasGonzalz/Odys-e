#pragma once

#include <cstddef>
#include <string>
#include <string_view>

namespace odyssee::net {

/// Code d'invitation (T056, V1 : docs/MODES.md) : un secret partagé que le client donne pendant
/// la poignée de main. Un serveur qui en a un refuse quiconque ne le connaît pas.
///
/// Forme : lettres et chiffres, 32 au plus. On le tape comme on le lit : les majuscules et
/// minuscules se valent, et les tirets et espaces sont ignorés (« ody-7kq2 » = « ODY7KQ2 »).

/// Longueur maximale d'un code, une fois normalisé.
inline constexpr std::size_t kMaxInviteCodeLength = 32;

/// Le code tel que comparé : en majuscules, sans tirets ni espaces.
[[nodiscard]] std::string normalizeInviteCode(std::string_view code);

/// Vrai si le code normalisé ne contient que des lettres ASCII et des chiffres, 32 au plus.
/// Le code vide est valide : c'est « pas de code ».
[[nodiscard]] bool isValidInviteCode(std::string_view code);

/// Vrai si `given` correspond au code du serveur `expected` (déjà normalisé). La comparaison
/// prend le même temps quel que soit le premier caractère faux : le temps de réponse ne
/// renseigne pas sur le code.
[[nodiscard]] bool inviteCodeMatches(std::string_view expected, std::string_view given);

} // namespace odyssee::net
