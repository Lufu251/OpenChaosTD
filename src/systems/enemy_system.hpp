#pragma once

#include <vector>
#include <engine/lib/dense_slotmap.hpp>
#include <engine/systems/particle_system.hpp>
#include <world/enemy.hpp>
#include <world/map.hpp>
#include <world/modules.hpp> // PendingChildSpawn

class EnemySystem{
public:
    void FollowPath(float dt, DenseSlotMap<Enemy>& enemies, const Map& map);
    // outSpawns collects any minions requested by modules this tick (e.g. SummonerModule); the world
    // layer places them after the tick so the enemy slotmap is not mutated while it is being walked.
    void TickEnemies(float dt, DenseSlotMap<Enemy>& enemies, const Map& map, ParticleSystem& particles,
                     std::vector<PendingChildSpawn>& outSpawns);
};
