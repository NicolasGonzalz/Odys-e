#include "StoryScreens.hpp"

#include <raylib.h>

#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

namespace odyssee::debugclient {

namespace {

constexpr Color kPanel{15, 18, 16, 225};
constexpr Color kPaper{235, 228, 205, 255};
constexpr Color kDim{170, 165, 150, 255};
constexpr Color kScreenGreen{120, 230, 140, 255};
constexpr int kText = 18;
constexpr int kLine = 24;

/// Écrit `text` en retournant à la ligne avant `width` pixels ; renvoie le y de la ligne suivante.
int drawWrapped(const std::string& text, int x, int y, int width, Color color) {
    std::istringstream paragraphs(text);
    std::string paragraph;
    while (std::getline(paragraphs, paragraph)) {
        std::istringstream words(paragraph);
        std::string line;
        std::string word;
        while (words >> word) {
            const std::string longer = line.empty() ? word : line + " " + word;
            if (!line.empty() && MeasureText(longer.c_str(), kText) > width) {
                DrawText(line.c_str(), x, y, kText, color);
                y += kLine;
                line = word;
            } else {
                line = longer;
            }
        }
        DrawText(line.c_str(), x, y, kText, color);
        y += kLine;
    }
    return y;
}

void drawCenteredPanel(int width, int height) {
    DrawRectangle((GetScreenWidth() - width) / 2, (GetScreenHeight() - height) / 2, width, height,
                  kPanel);
}

void drawSurvivorChoice(const data::StoryCatalog& story, const StoryState& state) {
    drawCenteredPanel(520, 260);
    const int x = GetScreenWidth() / 2 - 240;
    int y = GetScreenHeight() / 2 - 110;
    DrawText("Qui es-tu ?  (F1 à F4)", x, y, 24, kPaper);
    y += 44;
    for (const data::SurvivorDef& survivor : story.survivors()) {
        const bool taken = state.isTaken(survivor.id);
        const std::string line = "F" + std::to_string(survivor.id.value + 1) + "  " +
                                 survivor.name + (taken ? "  (déjà pris)" : "");
        DrawText(line.c_str(), x, y, 20, taken ? kDim : kPaper);
        y += 32;
    }
}

void drawIntro(const data::StoryCatalog& story, const StoryState& state) {
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Color{0, 0, 0, 235});
    int y = GetScreenHeight() / 3;
    for (const std::string& line : story.introFor(*state.survivor())) {
        DrawText(line.c_str(), GetScreenWidth() / 6, y, 22, kScreenGreen);
        y += 34;
    }
}

void drawReading(const data::StoryCatalog& story, const net::JournalEntry& entry) {
    const data::ClueDef* clue = story.clue({entry.clue});
    if (!clue) {
        return;
    }
    const int width = std::min(760, GetScreenWidth() - 40);
    const int x = (GetScreenWidth() - width) / 2;
    const int top = GetScreenHeight() - 300;
    DrawRectangle(x - 16, top - 16, width + 32, 270, kPanel);
    DrawText(clue->title.c_str(), x, top, 22, kPaper);
    const bool generic = entry.version == net::ClueVersion::Generic;
    int y = drawWrapped(generic ? clue->generic : clue->text, x, top + 36, width, kPaper);
    if (clue->personal() && !generic) {
        drawWrapped("Souvenir : " + clue->memory, x, y + 8, width, kScreenGreen);
    }
}

void drawJournal(const data::StoryCatalog& story, const StoryState& state) {
    const int width = std::min(900, GetScreenWidth() - 40);
    const int x = (GetScreenWidth() - width) / 2;
    DrawRectangle(x - 20, 20, width + 40, GetScreenHeight() - 40, kPanel);
    int y = 40;
    const data::SurvivorDef* self = state.survivor() ? story.survivor(*state.survivor()) : nullptr;
    DrawText(("Journal" + (self ? " de " + self->name : std::string())).c_str(), x, y, 26, kPaper);
    y += 44;
    DrawText("Souvenirs", x, y, 20, kScreenGreen);
    y = drawWrapped(self ? self->awakening : std::string("…"), x, y + 28, width, kPaper);
    for (const std::string& memory : state.memories(story)) {
        y = drawWrapped(memory, x, y, width, kPaper);
    }
    DrawText("Ce que j'ai compris", x, y + 12, 20, kScreenGreen);
    y += 40;
    for (const std::uint8_t number : state.understoodSteps(story)) {
        if (const data::StepDef* step = story.step(number)) {
            y = drawWrapped("- " + step->text, x, y, width, kPaper);
        }
    }
    const std::string count = "Indices lus : " + std::to_string(state.journal().size()) + " / " +
                              std::to_string(story.clues().size()) + "   (J : fermer)";
    DrawText(count.c_str(), x, y + 12, 18, kDim);
}

} // namespace

void drawStory(const data::StoryCatalog& story, const StoryState& state, double now) {
    if (state.introVisible(now) && state.survivor()) {
        drawIntro(story, state);
        return;
    }
    if (state.mustChoose()) {
        drawSurvivorChoice(story, state);
    }
    if (state.journalOpen()) {
        drawJournal(story, state);
        return;
    }
    if (const auto entry = state.reading(now)) {
        drawReading(story, *entry);
    }
    if (const auto notice = state.notice(now)) {
        const std::string owner = state.lockedOwnerName(story);
        const std::string text = *notice + (owner.empty() ? "" : " " + owner + " saurait le lire.");
        DrawText(text.c_str(), 20, GetScreenHeight() - 120, 20, kPaper);
    }
}

} // namespace odyssee::debugclient
