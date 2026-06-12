#pragma once

#include <raylib.h>
#include <vector>
#include <engine/systems/particle_system.hpp>
#include <engine/lib/dense_slotmap.hpp>

// Combat data: how an attack is styled, the status effects it applies, and the live attack
// (combat payload + visual) it spawns.

// --- Attack style ------------------------------------------------------------

// How an attack is drawn: a straight beam to each target (Line) or a single expanding ring (Ring).
enum class AttackStyle { Line, Ring };

// --- Effects -----------------------------------------------------------------

enum class EffectType {
    Burn,
    Slow,
    ArmorShred, // flat armor reduction while active (min 0)
    Stun,       // can't move; cleared when the enemy is hit
    Weakness    // next hit deals flat bonus damage, then is consumed
};

struct Effect {
public:
    EffectType m_type;
    float m_duration;
    float m_value; // per-type magnitude & "strength": Burn dps, Slow %, Shred amount, Weakness bonus dmg, Stun = its duration

    // Particle emission — set by the module that created this effect. m_particleDesc carries the
    // emit rate, shape, and dynamics; the live emitter is owned by the engine ParticleSystem and
    // referenced here by a stable handle (INVALID until first registered in EnemySystem).
    EmitterDesc m_particleDesc;
    EmitterHandle m_emitter = DenseSlotMap<Emitter>::INVALID_KEY;

    Effect(EffectType type, float duration, float value)
        : m_type(type), m_duration(duration), m_value(value) {}
};

// --- Attacks -----------------------------------------------------------------

// m_targetKeys only references DenseSlotMap<Enemy>::Key, which the template provides without the
// full Enemy definition; consumers that resolve a key to an Enemy include <world/enemy.hpp> directly.
class Enemy;

// Combat half of an attack: resolved once against each target, then left inert until the
// owning Attack's visual lifetime expires.
struct AttackPayload {
    std::vector<DenseSlotMap<Enemy>::Key> m_targetKeys;

    float m_damage = 0.0f;
    float m_armorPierce = 0.0f;
    float m_critChance = 0.0f;
    float m_critMultiplier = 1.0f;
    std::vector<Effect> m_effects;

    std::vector<EmitterDesc> m_impactDescs;
    std::vector<EmitterDesc> m_critImpactDescs;  // emitted in addition to m_impactDescs on crit

    bool m_resolved = false; // damage applied once; combat goes inert after this is set
};

// Visual half of an attack: how it is drawn. Target positions are a fire-time snapshot so lines
// keep rendering for the full lifetime even after the enemy moves or dies. Lifetime lives on Attack.
struct AttackVisual {
    Vector2 m_origin;
    std::vector<Vector2> m_targetPositions;
    AttackStyle m_style = AttackStyle::Line;
    Color m_color = {255, 220, 50, 255};
    float m_radius = 0.0f;
};

// One live attack: combat payload + its visual, sharing a single lifetime. Combat resolves once
// on the fire frame; the object persists until m_duration drains, driving the visual fade.
struct Attack {
    AttackPayload m_combat;
    AttackVisual m_visual;
    float m_duration = 0.0f;
    float m_maxDuration = 0.0f;

    float Progress() const {
        return (m_maxDuration > 0.0f) ? m_duration / m_maxDuration : 0.0f;
    }
};
