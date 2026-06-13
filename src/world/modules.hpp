#pragma once

#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <raylib.h>
#include <engine/systems/particle_system.hpp>
#include <world/desc_line.hpp>
#include <world/combat.hpp> // EffectType, Effect, AttackPayload

// All tower and enemy modules: the shared display/upgrade interface plus the two concrete module
// hierarchies. Tower modules contribute to attacks and fire-rate ramps; enemy modules contribute
// base stats, armor, shields and on-death behavior. The datapack-parse glue (ModuleDef) stays in
// world/module_def.hpp — it is shared with upgrades/factories and not used by the modules here.

// --- Shared display/upgrade interface ----------------------------------------

// Shared display/upgrade interface for tower and enemy modules. Both hierarchies append
// info-panel rows and accept upgrade patches through these two hooks with identical
// signatures; the behavioral hooks (firing, cloning, damage interception, …) stay on the
// concrete TowerModule / EnemyModule bases.
class IStatModule {
public:
    virtual ~IStatModule() = default;
    // Append this module's display lines to the info panel (zero or more rows).
    virtual void DescribeStats(std::vector<DescLine>&) const {}
    // Patch a module-owned parameter from the upgrade system (e.g. slowPercent, armor).
    virtual void PatchStats(const std::string& /*key*/, float /*v*/, bool /*mul*/) {}
};

// Apply an additive (mul=false) or multiplicative (mul=true) delta to one numeric field.
// Shared by AttackModule::PatchStat and every other module's PatchStat.
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

// --- Tower modules -----------------------------------------------------------

class AttackModule; // defined below; referenced by TowerModule::ContributeTower

enum class TargetingMode {
    First,
    Last,
    MostHealth,
    LowestHealth,
    Fastest,
    Slowest,
    MostArmor,
    MostShield,
    LowestEffectiveHealth, // health + shield: fewest shots to kill (unlike LowestHealth, counts shields)
    MostEffectiveHealth,   // health + shield: the true tank
    MostMaxHealth,         // by max health: stable "biggest unit" lock that doesn't thrash as HP drops
    MostReward,            // highest bounty (economy strategy)
    MostLives,             // highest livesOnReach: the biggest core threat on leak
    Closest,               // nearest to the tower
    Farthest               // farthest from the tower (within range)
};

inline constexpr int kTargetingModeCount = static_cast<int>(TargetingMode::Farthest) + 1; // Farthest stays last

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
        case TargetingMode::LowestEffectiveHealth: return "Least Eff HP";
        case TargetingMode::MostEffectiveHealth:   return "Most Eff HP";
        case TargetingMode::MostMaxHealth:  return "Most Max HP";
        case TargetingMode::MostReward:     return "Most Gold";
        case TargetingMode::MostLives:      return "Most Lives";
        case TargetingMode::Closest:        return "Closest";
        case TargetingMode::Farthest:       return "Farthest";
    }
    return "First";
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

// --- Enemy modules -----------------------------------------------------------

class Enemy;
class BaseStatsModule;

struct SpawnRequest {
    std::string m_type;
    int m_count;
    float m_spacing = 0.0f; // world-unit gap between consecutive children so they don't perfectly overlap
};

// A SpawnRequest captured during the enemy tick together with the originating enemy's path state, so
// the world layer can place the children mid-path (mirrors how OnDeath children inherit the parent's
// position/nest/waypoint). Collected by EnemySystem::TickEnemies and flushed after the iteration so
// the enemy slotmap is never mutated while it is being walked.
struct PendingChildSpawn {
    SpawnRequest m_request;
    Vector2 m_position;
    int m_nest;
    int m_waypointIndex;
    float m_progress;
};

// Damage-interception phase keys (lower runs first); see EnemyModule::InterceptOrder. Ordering by
// phase rather than module declaration order makes combat results independent of how a datapack
// lists an enemy's modules.
inline constexpr int kInterceptNegate  = 0;   // evasion, barrier — fully negate before anything is spent
inline constexpr int kInterceptAbsorb  = 10;  // shield pools — absorb flat damage
inline constexpr int kInterceptReduce  = 20;  // resistance — scale the remainder
inline constexpr int kInterceptDefault = 100; // pass-through modules (e.g. adrenaline trigger)

// DescribeStats/PatchStats are inherited from IStatModule (shared with TowerModule).
class EnemyModule : public IStatModule {
public:
    // Deep-copy this module (virtual-constructor idiom) so an Enemy prototype can be cloned.
    virtual std::unique_ptr<EnemyModule> Clone() const = 0;
    // Stateful per-frame hook (e.g. RegenerationModule); non-const so modules mutate cleanly. A module
    // that spawns minions while alive (e.g. SummonerModule) pushes onto outSpawns; the parent's path
    // state is attached and the children are placed by the world layer after the tick (see
    // PendingChildSpawn). Most modules ignore outSpawns.
    virtual void Tick(float /*dt*/, Enemy&, std::vector<SpawnRequest>& /*outSpawns*/) {}
    // Augment the enemy's live combat stats each tick (e.g. ArmorModule feeds m_liveArmor).
    // The mirror of TowerModule::ContributeTower(AttackModule&).
    virtual void ContributeStats(BaseStatsModule&) const {}
    // Stateful damage hook (e.g. ShieldModule depletes its pool); non-const for the same reason.
    virtual float InterceptDamage(float incoming) { return incoming; }
    // Phase key controlling the order InterceptDamage runs in (lower first); see kIntercept* above.
    virtual int InterceptOrder() const { return kInterceptDefault; }
    virtual std::optional<SpawnRequest> OnDeath() const { return std::nullopt; }
    virtual bool ShouldBlock(EffectType) const { return false; }
    virtual float GetShield() const { return 0.0f; }
};

// The "core" enemy module: owns an enemy's base stats (formerly the Enemy scalars + EnemyStats).
// The direct analogue of the tower's AttackModule. Base fields are the configured/upgraded values;
// the m_liveX mirror is reset from base each tick and augmented by other modules' ContributeStats
// (e.g. ArmorModule feeds m_liveArmor). Every enemy carries exactly one, built by EnemyFactory.
class BaseStatsModule : public EnemyModule {
public:
    // Base config — mutated only by upgrades via PatchStats.
    float m_maxHealth = 0.0f;
    float m_speed = 0.0f;
    int   m_reward = 0;
    int   m_livesOnReach = 1;

    // Live combat stats for the current tick. Reset from base by ResetLive(); armor has no innate
    // base (it is contributed entirely by ArmorModule). Current health is persistent runtime state
    // and lives on Enemy, so it is intentionally not reset here.
    float m_liveSpeed = 0.0f;
    float m_liveArmor = 0.0f;

    std::unique_ptr<EnemyModule> Clone() const override { return std::make_unique<BaseStatsModule>(*this); }
    void ResetLive(); // copy base -> live, called at the start of each recompute
    void PatchStats(const std::string& key, float v, bool mul) override;
    // Core stat rows (Health/Speed), reading the live values.
    void DescribeStats(std::vector<DescLine>& out) const override;
};

class RegenerationModule : public EnemyModule {
public:
    float m_rate;
    explicit RegenerationModule(float rate) : m_rate(rate) {}
    std::unique_ptr<EnemyModule> Clone() const override { return std::make_unique<RegenerationModule>(*this); }
    void Tick(float dt, Enemy& enemy, std::vector<SpawnRequest>& outSpawns) override;
    void DescribeStats(std::vector<DescLine>& out) const override;
    void PatchStats(const std::string& key, float v, bool mul) override;
};

class ArmorModule : public EnemyModule {
public:
    float m_amount;
    explicit ArmorModule(float amount) : m_amount(amount) {}
    std::unique_ptr<EnemyModule> Clone() const override { return std::make_unique<ArmorModule>(*this); }
    void ContributeStats(BaseStatsModule& base) const override;
    void DescribeStats(std::vector<DescLine>& out) const override;
    void PatchStats(const std::string& key, float v, bool mul) override;
};

// Enemy ignores any incoming effect of the given type
class ImmuneModule : public EnemyModule {
public:
    EffectType m_effect;
    explicit ImmuneModule(EffectType effect) : m_effect(effect) {}
    std::unique_ptr<EnemyModule> Clone() const override { return std::make_unique<ImmuneModule>(*this); }
    bool ShouldBlock(EffectType type) const override;
    void DescribeStats(std::vector<DescLine>& out) const override;
};

// Absorbs incoming damage until depleted
class ShieldModule : public EnemyModule {
public:
    float m_maxShield;
    float m_currentShield;
    explicit ShieldModule(float shield) : m_maxShield(shield), m_currentShield(shield) {}
    std::unique_ptr<EnemyModule> Clone() const override { return std::make_unique<ShieldModule>(*this); }
    float GetShield() const override { return m_currentShield; }
    float InterceptDamage(float incoming) override;
    int InterceptOrder() const override { return kInterceptAbsorb; }
    void DescribeStats(std::vector<DescLine>& out) const override;
    void PatchStats(const std::string& key, float v, bool mul) override;
};

// On death, spawns m_count children of type m_childType, staggered along the path by m_spacing so the
// children spread out instead of stacking on the exact death position.
class SplitModule : public EnemyModule {
public:
    std::string m_childType;
    int m_count;
    float m_spacing; // world-unit gap between consecutive children (0 = stack on the death position)
    SplitModule(std::string childType, int count, float spacing)
        : m_childType(std::move(childType)), m_count(count), m_spacing(spacing) {}
    std::unique_ptr<EnemyModule> Clone() const override { return std::make_unique<SplitModule>(*this); }
    std::optional<SpawnRequest> OnDeath() const override;
    void DescribeStats(std::vector<DescLine>& out) const override;
    void PatchStats(const std::string& key, float v, bool mul) override;
};

// Percentage damage reduction: scales every incoming hit by (1 - m_percent/100). The multiplicative
// complement to ArmorModule's flat reduction; composes with it (armor is subtracted first, in
// ResolveDamage, then this scales the remainder). m_percent is a percent (40 = 40% less damage).
class ResistanceModule : public EnemyModule {
public:
    float m_percent;
    explicit ResistanceModule(float percent) : m_percent(percent) {}
    std::unique_ptr<EnemyModule> Clone() const override { return std::make_unique<ResistanceModule>(*this); }
    float InterceptDamage(float incoming) override;
    int InterceptOrder() const override { return kInterceptReduce; }
    void DescribeStats(std::vector<DescLine>& out) const override;
    void PatchStats(const std::string& key, float v, bool mul) override;
};

// Chance to fully negate a hit. The roll uses the same global RNG stream as the tower crit roll
// (raylib's GetRandomValue), so a save replay stays deterministic. m_dodgeChance is in [0..1].
class EvasionModule : public EnemyModule {
public:
    float m_dodgeChance;
    explicit EvasionModule(float dodgeChance) : m_dodgeChance(dodgeChance) {}
    std::unique_ptr<EnemyModule> Clone() const override { return std::make_unique<EvasionModule>(*this); }
    float InterceptDamage(float incoming) override;
    int InterceptOrder() const override { return kInterceptNegate; }
    void DescribeStats(std::vector<DescLine>& out) const override;
    void PatchStats(const std::string& key, float v, bool mul) override;
};

// Fully blocks the next m_maxHits hits regardless of their magnitude, then lets damage through.
// Distinct from ShieldModule, which absorbs a damage pool: a Barrier charge eats an entire hit, big
// or small. m_hitsLeft is the live counterpart that each blocked hit decrements.
class BarrierModule : public EnemyModule {
public:
    int m_maxHits;
    int m_hitsLeft;
    explicit BarrierModule(int hitCount) : m_maxHits(hitCount), m_hitsLeft(hitCount) {}
    std::unique_ptr<EnemyModule> Clone() const override { return std::make_unique<BarrierModule>(*this); }
    float InterceptDamage(float incoming) override;
    int InterceptOrder() const override { return kInterceptNegate; }
    void DescribeStats(std::vector<DescLine>& out) const override;
    void PatchStats(const std::string& key, float v, bool mul) override;
};

// Speeds up once health drops below m_healthThreshold (fraction of max). Tick latches the active
// flag from current/max health; ContributeStats adds the bonus speed to the live mirror while active.
// A counter to slow-DPS strategies that let enemies linger at low health.
class EnrageModule : public EnemyModule {
public:
    float m_healthThreshold; // fraction of max health (0.3 = enrage below 30%)
    float m_speedBonus;
    bool m_active = false;
    EnrageModule(float healthThreshold, float speedBonus)
        : m_healthThreshold(healthThreshold), m_speedBonus(speedBonus) {}
    std::unique_ptr<EnemyModule> Clone() const override { return std::make_unique<EnrageModule>(*this); }
    void Tick(float dt, Enemy& enemy, std::vector<SpawnRequest>& outSpawns) override;
    void ContributeStats(BaseStatsModule& base) const override;
    void DescribeStats(std::vector<DescLine>& out) const override;
    void PatchStats(const std::string& key, float v, bool mul) override;
};

// A shield pool that recharges after a damage-free delay. Like ShieldModule it absorbs incoming
// damage (InterceptDamage), but each hit resets the recharge timer; once m_rechargeDelay seconds pass
// without a hit, the pool refills at m_rechargeRate per second. Kept separate from ShieldModule.
class ShieldRegenModule : public EnemyModule {
public:
    float m_maxShield;
    float m_currentShield;
    float m_rechargeRate;
    float m_rechargeDelay;
    float m_timeSinceHit = 0.0f; // live: seconds since the last absorbed hit
    ShieldRegenModule(float shield, float rechargeRate, float rechargeDelay)
        : m_maxShield(shield), m_currentShield(shield),
          m_rechargeRate(rechargeRate), m_rechargeDelay(rechargeDelay) {}
    std::unique_ptr<EnemyModule> Clone() const override { return std::make_unique<ShieldRegenModule>(*this); }
    float GetShield() const override { return m_currentShield; }
    float InterceptDamage(float incoming) override;
    int InterceptOrder() const override { return kInterceptAbsorb; }
    void Tick(float dt, Enemy& enemy, std::vector<SpawnRequest>& outSpawns) override;
    void DescribeStats(std::vector<DescLine>& out) const override;
    void PatchStats(const std::string& key, float v, bool mul) override;
};

// Temporary speed burst triggered by taking a hit: InterceptDamage (re)arms a timer (damage passes
// through unchanged), Tick counts it down, and ContributeStats adds the bonus speed while it runs.
class AdrenalineModule : public EnemyModule {
public:
    float m_speedBonus;
    float m_duration;
    float m_timer = 0.0f; // live: seconds of boost remaining
    AdrenalineModule(float speedBonus, float duration)
        : m_speedBonus(speedBonus), m_duration(duration) {}
    std::unique_ptr<EnemyModule> Clone() const override { return std::make_unique<AdrenalineModule>(*this); }
    float InterceptDamage(float incoming) override;
    void Tick(float dt, Enemy& enemy, std::vector<SpawnRequest>& outSpawns) override;
    void ContributeStats(BaseStatsModule& base) const override;
    void DescribeStats(std::vector<DescLine>& out) const override;
    void PatchStats(const std::string& key, float v, bool mul) override;
};

// Periodically spawns m_count children of type m_childType while alive (every m_interval seconds),
// staggered by m_spacing like SplitModule's death children. Tick advances the timer and pushes a
// SpawnRequest onto the per-tick out-param; the world layer places the children mid-path.
class SummonerModule : public EnemyModule {
public:
    std::string m_childType;
    int m_count;
    float m_spacing;
    float m_interval;
    float m_timer = 0.0f; // live: seconds accumulated toward the next summon
    SummonerModule(std::string childType, int count, float spacing, float interval)
        : m_childType(std::move(childType)), m_count(count), m_spacing(spacing), m_interval(interval) {}
    std::unique_ptr<EnemyModule> Clone() const override { return std::make_unique<SummonerModule>(*this); }
    void Tick(float dt, Enemy& enemy, std::vector<SpawnRequest>& outSpawns) override;
    void DescribeStats(std::vector<DescLine>& out) const override;
    void PatchStats(const std::string& key, float v, bool mul) override;
};
