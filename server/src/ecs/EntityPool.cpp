#include "ecs/EntityPool.hpp"

namespace odyssee::server::ecs {

Entity EntityPool::create() {
    if (!_free.empty()) {
        const std::uint32_t index = _free.back();
        _free.pop_back();
        _used[index] = true;
        return {index, _generations[index]};
    }
    const auto index = static_cast<std::uint32_t>(_generations.size());
    _generations.push_back(0);
    _used.push_back(true);
    return {index, 0};
}

void EntityPool::destroy(Entity entity) {
    if (!alive(entity)) {
        return;
    }
    _used[entity.index] = false;
    ++_generations[entity.index]; // les anciens identifiants ne désignent plus rien
    _free.push_back(entity.index);
}

bool EntityPool::alive(Entity entity) const {
    return entity.index < _generations.size() && _used[entity.index] &&
           _generations[entity.index] == entity.generation;
}

} // namespace odyssee::server::ecs
