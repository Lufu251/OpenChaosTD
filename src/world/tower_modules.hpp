#pragma once

#include <string>
#include <vector>
#include <raylib.h>
#include <engine/systems/particle_system.hpp>
#include <world/desc_line.hpp>
#include <world/stat_module.hpp>

struct AttackPayload;
class AttackModule;

// --- Shared module utilities (formerly tower_stats.hpp) ---

enum class TargetingMode {
    First,
    Last,
    MostHealth,
    LowestHealth,
    Fastest,
    Slowest,
    MostArmor,
    MostShield
};

inline constexpr int kTargetingModeCount = static_cast<int>(TargetingMode::MostShield) + 1; // MostShield stays last

inline TargetingMode NextTargetingMode(TargetingMode m) {
    return static_cast<TargetingMode>((static_cast<int>(m) + 1) % kTargetingModeCount);
}

inline const char* TargetingModeName(TargetingMode m) {
    switch (m) {
        case TargetingMode::First:          return "First";
        case TargetingMode::Last:           return "Last";
        case TargetingMode::MostHealth:     return "Most HP";
        case TargetingMode::LowestHealth:   return "Least HP";
        case TargetingMode::Fastest:        return "Fastest";
        case TargetingMode::Slowest:        return "Slowest";
        case TargetingMode::MostArmor:      return "Most Armor";
        case TargetingMode::MostShield:     return "Most Shield";
    }
    return "First";
}

// Apply an additive (mul=false) or multiplicative (mul=true) delta to one numeric field.
// Shared by AttackModule::PatchStat and every other TowerModule::PatchStat.
inline void ApplyDelta(float& field, float v, bool mul) {
    field = mul ? field * v : field + v;
}

// Apply a delta to an integer field via the float path, rounding half-up. The exact
// `+ 0.5f` rounding is load-bearing for save-replay determinism; do not change it.
inline void PatchInt(int& field, float v, bool mul) {
    float t = static_cast<float>(field);
    ApplyDelta(t, v, mul);
    field = static_cast<int>(t + 0.5f);
}

// DescribeStats/PatchStats are inherited from IStatModule (shared with EnemyModule).
class TowerModule : public IStatModule {
public:
    virtual void Contribute(AttackPayload&) const {}
    // Augment the firing tower's live combat stats each tick (e.g. RampUp's fire-rate ramp).
    virtual void ContributeTower(AttackModule&) const {}
    // Stateful hooks: Tick runs every frame (idle decay), OnFire runs the frame the tower fires.
    virtual void Tick(float /*dt*/) {}
    virtual void OnFire() {}
};

// The "shooter" module: owns a tower's core combat stats (formerly TowerStats). A tower with an
// AttackModule attacks; one without (e.g. a Wall carrying only a PassiveModule) never fires.
// Base fields are the configured/upgraded values; the m_liveX mirror is recomputed each tick from
// base + other modules' ContributeTower contributions and is what the systems read.
class AttackModule : public TowerModule {
public:
    // Base combat config — mutated only by upgrades via PatchStat.
    float m_damage = 0.0f;
    float m_shotsPerMinute = 0.0f; // cooldown between shots = 60 / m_shotsPerMinute
    float m_range = 0.0f;
    int m_targetCount = 0;
    TargetingMode m_targetingMode = TargetingMode::First;

    // Live values for the current tick (base + contributions). Reset from base by ResetLive().
    // Targeting mode is never contributed, so readers use m_targetingMode directly.
    float m_liveDamage = 0.0f;
    float m_liveShotsPerMinute = 0.0f;
    float m_liveRange = 0.0f;
    int m_liveTargetCount = 0;

    void ResetLive(); // copy base -> live, called at the start of each recompute
    void PatchStats(const std::string& key, float v, bool mul) override;
    // Core stat rows (Damage/Range/Rate/Targets), reading the live values.
    void DescribeStats(std::vector<DescLine>& out) const override;
};

// The "wall" module: a pure marker for a non-attacking blocker. Carries no stats; a tower is a
// wall precisely because it has no AttackModule.
class PassiveModule : public TowerModule {
public:
};

// Flat armor penetration: ignores up to m_amount of the target's armor before damage reduction.
// Contributes to the attack payload, so it composes onto any AttackModule tower.
class ArmorPierceModule : public TowerModule {
public:
    float m_amount = 0.0f; // flat armor ignored before reduction
    explicit ArmorPierceModule(float amount);
    void Contribute(AttackPayload& attack) const override;
    void DescribeStats(std::vector<DescLine>& out) const override;
    void PatchStats(const std::string& key, float v, bool mul) override;
};

class SlowModule : public TowerModule {
public:
    float m_slowPercent, m_duration; // m_slowPercent: slow strength as a percent (90 = 90% slower)
    EmitterDesc m_particleDesc;
    SlowModule(float slowPercent, float duration, EmitterDesc particleDesc);
    void Contribute(AttackPayload& attack) const override;
    void DescribeStats(std::vector<DescLine>& out) const override;
    void PatchStats(const std::string& key, float v, bool mul) override;
};

class BurnModule : public TowerModule {
public:
    float m_damage, m_duration;
    EmitterDesc m_particleDesc;
    BurnModule(float value, float duration, EmitterDesc particleDesc);
    void Contribute(AttackPayload& attack) const override;
    void DescribeStats(std::vector<DescLine>& out) const override;
    void PatchStats(const std::string& key, float v, bool mul) override;
};

class ArmorShredModule : public TowerModule {
public:
    float m_amount, m_duration; // m_amount: flat armor removed while active
    EmitterDesc m_particleDesc;
    ArmorShredModule(float amount, float duration, EmitterDesc particleDesc);
    void Contribute(AttackPayload& attack) const override;
    void DescribeStats(std::vector<DescLine>& out) const override;
    void PatchStats(const std::string& key, float v, bool mul) override;
};

class WeaknessModule : public TowerModule {
public:
    float m_amount, m_duration; // m_amount: flat bonus damage the next hit deals
    EmitterDesc m_particleDesc;
    WeaknessModule(float amount, float duration, EmitterDesc particleDesc);
    void Contribute(AttackPayload& attack) const override;
    void DescribeStats(std::vector<DescLine>& out) const override;
    void PatchStats(const std::string& key, float v, bool mul) override;
};

class StunModule : public TowerModule {
public:
    float m_duration;
    EmitterDesc m_particleDesc;
    StunModule(float duration, EmitterDesc particleDesc);
    void Contribute(AttackPayload& attack) const override;
    void DescribeStats(std::vector<DescLine>& out) const override;
    void PatchStats(const std::string& key, float v, bool mul) override;
};

// Supplies the attack's critical-hit chance and multiplier (the roll happens in damage resolution)
class CritModule : public TowerModule {
public:
    float m_critChance, m_critMultiplier; // m_critChance in [0..1]; m_critMultiplier applied on a crit
    CritModule(float critChance, float critMultiplier);
    void Contribute(AttackPayload& attack) const override;
    void DescribeStats(std::vector<DescLine>& out) const override;
    void PatchStats(const std::string& key, float v, bool mul) override;
};

// Self-buff: each shot adds a stack (capped) that raises fire rate; stacks clear after idle time
class RampUpModule : public TowerModule {
public:
    float m_bonusPerStack, m_idleTime;
    int m_maxStacks;
    int m_stacks = 0;
    float m_idleTimer = 0.0f;
    RampUpModule(float bonusPerStack, int maxStacks, float idleTime);
    void ContributeTower(AttackModule& attack) const override;
    void Tick(float dt) override;
    void OnFire() override;
    void DescribeStats(std::vector<DescLine>& out) const override;
    void PatchStats(const std::string& key, float v, bool mul) override;
};
