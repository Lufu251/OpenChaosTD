#pragma once

#include <raylib.h>
#include <cstddef>
#include <string>
#include <vector>
#include <memory>
#include <engine/systems/particle_system.hpp> // EmitterDesc
#include <world/combat.hpp>  // Effect, EffectType
#include <world/modules.hpp> // EnemyModule, BaseStatsModule, ShieldModule
#include <world/upgrade.hpp> // EnemyUpgrade

// The complete presentation (visuals + audio) of an enemy. Set once by EnemyFactory, never modified at runtime.
struct EnemyPresentation {
    std::string m_texture;                       // resource key for the enemy sprite
    std::string m_deathSound = "enemy_death";    // resource key for the death sfx; defaults to enemy_death
    const EmitterDesc* m_deathDescPtr = nullptr; // stable pointer into EmitterPresets; burst emitted on death
};

class Enemy {
public:
    std::string m_name;
    std::string m_description;
    Vector2 m_position = {0.0f, 0.0f};

    EnemyPresentation m_presentation; // set by factory, never modified at runtime

    // Runtime hit points. Max/speed/reward/livesOnReach live in the BaseStatsModule (see GetBaseStats);
    // current health is the live counterpart that damage and regen mutate, like the tower's m_cooldown.
    float m_currentHealth = 0.0f;

    float m_progress = 0.0f;
    int   m_spawnedNest   = 0;
    int   m_waypointIndex = -1;

    std::vector<Effect> m_effects;
    std::vector<std::unique_ptr<EnemyModule>> m_modules;

    int m_level = 1; // base tier; EnemyFactory::ApplyUpgrade bumps this per upgrade tier applied
    const EnemyUpgrade* m_upgrade = nullptr; // stable pointer into the factory template (null if none)

    // Patch a stat by key: broadcast to every module (the BaseStatsModule handles the core stats,
    // the rest handle shield, armor, regenRate, splitCount, ...). Mirrors the generic PatchStats
    // pipeline; ApplyDelta is shared from world/modules.hpp.
    void PatchStats(const std::string& key, float v, bool mul) {
        for (auto& mod : m_modules)
            mod->PatchStats(key, v, mul);
        // A scaled/elite enemy spawns at full HP, so re-sync current health after maxHealth changes.
        if (key == "maxHealth" && m_baseStats)
            m_currentHealth = m_baseStats->m_maxHealth;
    }

    // Deep-copy this enemy (modules included). Used to spawn from a pre-upgraded prototype and to
    // hand fully-upgraded copies to the HUD. Enemy is move-only (its modules are unique_ptr), so the
    // implicit copy is deleted; cloning rebuilds each module via its virtual Clone() and re-adds it
    // through AddModule, which re-caches m_baseStats/m_shield into the new module vector (order
    // preserved). m_upgrade is a stable pointer into the factory template, so a shallow copy is
    // correct (mirrors EnemyFactory::Create); m_level is copied as-is.
    Enemy Clone() const {
        // When adding a field to Enemy, add the matching copy line below. The static_assert on
        // kExpectedEnemySize (just after the class) fires when sizeof(Enemy) changes, forcing a
        // visit here so a new field is never silently dropped from the clone.
        Enemy copy;
        copy.m_name          = m_name;
        copy.m_description    = m_description;
        copy.m_position       = m_position;
        copy.m_presentation   = m_presentation;
        copy.m_currentHealth  = m_currentHealth;
        copy.m_progress       = m_progress;
        copy.m_spawnedNest    = m_spawnedNest;
        copy.m_waypointIndex  = m_waypointIndex;
        copy.m_effects        = m_effects;
        copy.m_level          = m_level;
        copy.m_upgrade        = m_upgrade;
        for (const auto& mod : m_modules)
            copy.AddModule(mod->Clone());
        return copy;
    }

    void AddModule(std::unique_ptr<EnemyModule> mod) {
        // Cache the BaseStatsModule and ShieldModule (if present) so the damage and targeting hot
        // paths can read core/shield stats without scanning the module list every frame.
        if (auto* base = dynamic_cast<BaseStatsModule*>(mod.get()))
            m_baseStats = base;
        if (auto* shield = dynamic_cast<ShieldModule*>(mod.get()))
            m_shield = shield;
        m_modules.push_back(std::move(mod));
    }

    // Always non-null for a factory-built enemy; points into m_modules, set in AddModule. Owns the
    // core stats (maxHealth/speed/reward/livesOnReach) and the live speed/armor mirror.
    BaseStatsModule* GetBaseStats() const { return m_baseStats; }

    // Non-null iff this enemy carries a shield; points into m_modules, set in AddModule.
    ShieldModule* GetShield() const { return m_shield; }

    // Recompute the live combat stats from base + module contributions (speed, armor). Shared by
    // the per-tick update, the factory build-time prime, and the wave-preview path so the three
    // never drift. The enemy analogue of TowerSystem::RecomputeStats.
    void RecomputeLive() {
        if (!m_baseStats) return;
        m_baseStats->ResetLive();
        for (auto& mod : m_modules)
            mod->ContributeStats(*m_baseStats);
    }

    void AddEffect(Effect effect) {
        for (const auto& mod : m_modules)
            if (mod->ShouldBlock(effect.m_type)) return;

        // Effects don't stack; reapply only refreshes (timer + value) when equal or stronger.
        // Preserve the running emitter handle so the refresh doesn't orphan the live emitter.
        for (auto& existing : m_effects) {
            if (existing.m_type != effect.m_type) continue;
            if (effect.m_value >= existing.m_value) {
                EmitterHandle keep = existing.m_emitter;
                existing = effect;
                existing.m_emitter = keep;
            }
            return;
        }
        m_effects.push_back(std::move(effect));
    }

    Effect* FindEffect(EffectType type) {
        for (auto& e : m_effects)
            if (e.m_type == type) return &e;
        return nullptr;
    }

    void RemoveEffect(EffectType type) {
        std::erase_if(m_effects, [type](const Effect& e){ return e.m_type == type; });
    }

private:
    BaseStatsModule* m_baseStats = nullptr; // points into m_modules; set in AddModule
    ShieldModule* m_shield = nullptr;       // points into m_modules; set in AddModule
};

// Tripwire for Clone(): adding/removing/reordering an Enemy field changes sizeof(Enemy) and trips
// this assert, forcing a look at Clone() so the new field gets a copy line. Update the constant
// once Clone() is handled. The size is the 64-bit layout, so the check is scoped to 64-bit
// builds (the normal dev target); 32-bit targets such as the wasm32 web build have smaller
// pointers and a different sizeof, where this tripwire does not apply.
inline constexpr std::size_t kExpectedEnemySize = 240;
static_assert(sizeof(void*) != 8 || sizeof(Enemy) == kExpectedEnemySize,
              "Enemy layout changed: update Enemy::Clone() for the new field, then set "
              "kExpectedEnemySize to sizeof(Enemy).");
