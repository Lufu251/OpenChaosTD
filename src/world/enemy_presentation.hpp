#pragma once

#include <string>
#include <engine/systems/particle_system.hpp> // EmitterDesc

// The complete presentation (visuals + audio) of an enemy. Set once by EnemyFactory, never modified at runtime.
struct EnemyPresentation {
    std::string m_texture;                       // resource key for the enemy sprite
    std::string m_deathSound = "enemy_death";    // resource key for the death sfx; defaults to enemy_death
    const EmitterDesc* m_deathDescPtr = nullptr; // stable pointer into EmitterPresets; burst emitted on death
};
