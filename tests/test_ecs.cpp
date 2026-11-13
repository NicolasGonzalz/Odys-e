#include "ecs/Registry.hpp"

#include <doctest/doctest.h>

#include <string>
#include <vector>

using namespace odyssee::server::ecs;

namespace {

struct Position {
    int x = 0;
    int y = 0;
};

struct Name {
    std::string text;
};

struct Health {
    int points = 100;
};

} // namespace

TEST_CASE("ECS : une entité détruite ne revient pas, même si sa place est reprise") {
    EntityPool pool;
    const Entity first = pool.create();
    const Entity second = pool.create();
    CHECK(pool.size() == 2);
    CHECK_FALSE(first == second);

    pool.destroy(first);
    CHECK_FALSE(pool.alive(first));
    CHECK(pool.size() == 1);

    const Entity reused = pool.create(); // même place, nouvelle génération
    CHECK(reused.index == first.index);
    CHECK(reused.generation != first.generation);
    CHECK(pool.alive(reused));
    CHECK_FALSE(pool.alive(first));

    pool.destroy(first); // ancien identifiant : sans effet sur la nouvelle entité
    CHECK(pool.alive(reused));
}

TEST_CASE("ECS : ajouter, lire, remplacer et retirer un composant") {
    Registry world;
    const Entity tree = world.create();

    world.add<Position>(tree, 3, 4);
    REQUIRE(world.get<Position>(tree) != nullptr);
    CHECK(world.get<Position>(tree)->x == 3);
    CHECK_FALSE(world.has<Health>(tree));

    world.add<Position>(tree, 7, 8); // remplace
    CHECK(world.pool<Position>().size() == 1);
    CHECK(world.get<Position>(tree)->y == 8);

    CHECK(world.remove<Position>(tree));
    CHECK(world.get<Position>(tree) == nullptr);
    CHECK_FALSE(world.remove<Position>(tree));
}

TEST_CASE("ECS : retirer un composant au milieu garde les autres à jour") {
    Registry world;
    std::vector<Entity> rocks;
    for (int i = 0; i < 5; ++i) {
        rocks.push_back(world.create());
        world.add<Position>(rocks.back(), i, 0);
    }
    world.remove<Position>(rocks[1]); // le dernier prend sa place dans le tableau serré

    for (int i : {0, 2, 3, 4}) {
        REQUIRE(world.get<Position>(rocks[static_cast<std::size_t>(i)]) != nullptr);
        CHECK(world.get<Position>(rocks[static_cast<std::size_t>(i)])->x == i);
    }
}

TEST_CASE("ECS : détruire une entité retire tous ses composants") {
    Registry world;
    const Entity wolf = world.create();
    world.add<Position>(wolf);
    world.add<Health>(wolf);

    world.destroy(wolf);
    CHECK_FALSE(world.alive(wolf));
    CHECK(world.pool<Position>().size() == 0);
    CHECK(world.pool<Health>().size() == 0);

    // La place reprise par une autre entité ne retrouve pas les anciens composants.
    const Entity rabbit = world.create();
    CHECK(rabbit.index == wolf.index);
    CHECK_FALSE(world.has<Health>(rabbit));
}

TEST_CASE("ECS : each ne visite que les entités qui ont tous les composants demandés") {
    Registry world;
    const Entity a = world.create();
    const Entity b = world.create();
    const Entity c = world.create();
    world.add<Position>(a);
    world.add<Position>(b);
    world.add<Health>(b, 50);
    world.add<Health>(c, 10);

    int visited = 0;
    world.each<Position, Health>([&](Entity entity, Position& position, Health& health) {
        CHECK(entity == b);
        position.x = health.points; // modifie bien le composant stocké
        ++visited;
    });
    CHECK(visited == 1);
    CHECK(world.get<Position>(b)->x == 50);
}

TEST_CASE("ECS : un système peut détruire les entités qu'il parcourt") {
    Registry world;
    for (int points : {0, 30, 0, 80, 0}) {
        world.add<Health>(world.create(), points);
    }

    // « Mort » : les entités sans points de vie disparaissent pendant le parcours.
    int visited = 0;
    world.each<Health>([&](Entity entity, Health& health) {
        ++visited;
        if (health.points == 0) {
            world.destroy(entity);
        }
    });
    CHECK(visited == 5);
    CHECK(world.size() == 2);
    CHECK(world.pool<Health>().size() == 2);
}

TEST_CASE("ECS : composants non copiables (chaînes) déplacés correctement") {
    Registry world;
    const Entity first = world.create();
    const Entity second = world.create();
    world.add<Name>(first, Name{"Ilian"});
    world.add<Name>(second, Name{"Nadia"});
    world.remove<Name>(first);
    REQUIRE(world.get<Name>(second) != nullptr);
    CHECK(world.get<Name>(second)->text == "Nadia");
}
