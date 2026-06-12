#pragma once

#include <engine/lib/dense_slotmap.hpp>
#include <engine/systems/particle_system.hpp>
#include <world/enemy.hpp>
#include <world/map.hpp>

class EnemySystem{
public:
    void FollowPath(float dt, DenseSlotMap<Enemy>& enemies, const Map& map);
    void TickEnemies(float dt, DenseSlotMap<Enemy>& enemies, const Map& map, ParticleSystem& particles);
};
