#pragma once

#include <raylib.h>
#include <string>
#include <world/attack_style.hpp>
#include <engine/systems/particle_system.hpp> // EmitterDesc

// Per-frame visual state of a tower (animation), separate from the immutable descriptor below.
// Lives with the presentation, not the simulation, so the Tower struct holds no rendering fields.
struct TowerAnimation {
    float m_attackFlashRatio = 0.0f; // 1.0 on fire, decays to 0 over TowerPresentation::m_attackDuration
};

// The complete presentation (visuals + audio) of a tower. Set once by TowerFactory, never modified at runtime.
struct TowerPresentation {
    std::string m_texture; // resource key for the tower sprite
    std::string m_attackSound; // resource key for the attack sfx; empty = silent
    AttackStyle m_style = AttackStyle::Line;
    Color m_color = {255, 220, 50, 255};
    float m_attackDuration = 0.0f; // how long the beam/ring shows and the muzzle-flash fades

    EmitterDesc m_muzzleDesc;     // burst at the tower when it fires
    EmitterDesc m_impactDesc;     // burst on each enemy hit
    EmitterDesc m_critImpactDesc; // extra burst on a crit hit
};
