#pragma once

#include <engine/systems/particle_system.hpp>

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
