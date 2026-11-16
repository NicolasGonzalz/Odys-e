#pragma once

#include "odyssee/data/StoryCatalog.hpp"
#include "odyssee/net/GameEvents.hpp"

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace odyssee::debugclient {

/// Ce que le client sait de l'histoire (T049), d'après les événements du serveur : son survivant,
/// son journal, et ce qu'il faut montrer en ce moment (écran du réveil, indice lu, message).
/// Sans dessin : StoryScreens s'en charge. `now` : secondes (GetTime de raylib).
class StoryState {
  public:
    /// Durée d'affichage de l'écran du réveil et d'un indice lu.
    static constexpr double kIntroSeconds = 10.0;
    static constexpr double kReadingSeconds = 14.0;

    /// Traite un événement du serveur s'il concerne l'histoire ; faux sinon.
    bool onEvent(const net::GameEvent& event, double now);

    [[nodiscard]] std::optional<data::SurvivorId> survivor() const;
    [[nodiscard]] bool isTaken(data::SurvivorId survivor) const;
    /// Vrai tant que le serveur n'a pas dit qui l'on est, ou qu'on n'a pas choisi.
    [[nodiscard]] bool mustChoose() const { return _statusKnown && !survivor(); }

    [[nodiscard]] bool introVisible(double now) const { return now < _introUntil; }
    /// L'indice qu'on vient de lire, tant qu'il reste affiché.
    [[nodiscard]] std::optional<net::JournalEntry> reading(double now) const;
    /// Message court (indice verrouillé), tant qu'il reste affiché.
    [[nodiscard]] std::optional<std::string> notice(double now) const;

    [[nodiscard]] std::span<const net::JournalEntry> journal() const { return _journal; }
    /// Étapes du fil comprises (au moins un indice lu qui la révèle), dans l'ordre.
    [[nodiscard]] std::vector<std::uint8_t> understoodSteps(const data::StoryCatalog& story) const;
    /// Souvenirs retrouvés : ceux des indices personnels lus par soi-même.
    [[nodiscard]] std::vector<std::string> memories(const data::StoryCatalog& story) const;

    void toggleJournal() { _journalOpen = !_journalOpen; }
    [[nodiscard]] bool journalOpen() const { return _journalOpen; }
    /// Le nom affiché d'un survivant cité dans un message (vide si inconnu).
    std::string lockedOwnerName(const data::StoryCatalog& story) const;

  private:
    void onStatus(const net::SurvivorStatus& status, double now);
    void onEntry(const net::JournalEntry& entry, double now);

    bool _statusKnown = false;
    net::SurvivorStatus _status;
    std::vector<net::JournalEntry> _journal;
    std::optional<net::JournalEntry> _reading;
    std::optional<net::ClueLocked> _locked;
    double _greetedAt = -1.0; ///< réception du premier statut (le journal initial suit)
    double _introUntil = 0.0;
    double _readingUntil = 0.0;
    double _noticeUntil = 0.0;
    bool _journalOpen = false;
};

} // namespace odyssee::debugclient
