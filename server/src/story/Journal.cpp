#include "story/Journal.hpp"

#include <algorithm>

namespace odyssee::server {

bool Journal::add(const JournalRecord& record) {
    if (has(record.clue, record.version)) {
        return false;
    }
    _records.push_back(record);
    return true;
}

bool Journal::has(data::ClueId clue, net::ClueVersion version) const {
    return std::any_of(_records.begin(), _records.end(), [&](const JournalRecord& record) {
        return record.clue == clue && record.version == version;
    });
}

} // namespace odyssee::server
