#include <systems/tower_system.hpp>

#include <raymath.h>
#include <algorithm>

#include <world/tower.hpp>
#include <world/modules.hpp>
#include <engine/systems/sound_system.hpp>

// Cooldown sentinels (seconds). Cooldown counts down each frame; a shot is taken at <= 0.
constexpr float kIdleRetryCooldown = 0.05f;  // short retry while no target is in range
constexpr float kInactiveCooldown  = 999.0f; // shotsPerMinute == 0: effectively never fires
constexpr float kSecondsPerMinute  = 60.0f;  // shot interval = 60 / shotsPerMinute

void TowerSystem::Update(float dt, DenseSlotMap<Tower>& towers, DenseSlotMap<Enemy>& enemies, std::vector<Attack>& attacks, ParticleSystem& particles, SoundSystem& sound){
    for (Tower& tower : towers) {
        RecomputeStats(tower, dt); // runs for walls too (ticks their modules)

        AttackModule* attack = tower.GetAttack();
        if (!attack) continue; // no AttackModule -> a wall, never fires

        tower.m_cooldown -= dt;
        DecayAttackFlash(tower, dt);

        if (tower.m_cooldown > 0.0f) continue; // not ready to fire yet

        std::vector<DenseSlotMap<Enemy>::Key> targetKeys = FindTargets(tower, enemies, attack->m_liveTargetCount);
        if (targetKeys.empty()) {
            tower.m_cooldown = kIdleRetryCooldown;
            continue;
        }

        Fire(tower, targetKeys, enemies, attacks, particles, sound);
    }
}

// Reset the AttackModule's live stats from base, then let modules tick and augment them.
void TowerSystem::RecomputeStats(Tower& tower, float dt) {
    AttackModule* attack = tower.GetAttack();
    if (attack) attack->ResetLive();
    for (auto& mod : tower.m_modules) {
        mod->Tick(dt);
        if (attack) mod->ContributeTower(*attack); // AttackModule's own override is a no-op
    }
}

// Decay the attack flash over the visual's m_attackDuration; guard against a zero duration.
void TowerSystem::DecayAttackFlash(Tower& tower, float dt) {
    if (tower.m_presentation.m_attackDuration > 0.0f)
        tower.m_animation.m_attackFlashRatio = std::max(0.0f, tower.m_animation.m_attackFlashRatio - dt / tower.m_presentation.m_attackDuration);
    else
        tower.m_animation.m_attackFlashRatio = 0.0f;
}

// Execute one shot: reset cooldown/flash, notify modules, and enqueue the damage payload + VFX.
void TowerSystem::Fire(Tower& tower, const std::vector<DenseSlotMap<Enemy>::Key>& targetKeys, DenseSlotMap<Enemy>& enemies, std::vector<Attack>& attacks, ParticleSystem& particles, SoundSystem& sound) {
    float shotsPerMinute     = tower.GetAttack()->m_liveShotsPerMinute;
    tower.m_cooldown = (shotsPerMinute > 0.0f) ? kSecondsPerMinute / shotsPerMinute : kInactiveCooldown;
    tower.m_animation.m_attackFlashRatio = 1.0f;
    for (auto& mod : tower.m_modules) // note: after the cooldown reset, so a new stack affects the next-but-one shot
        mod->OnFire();

    std::vector<Vector2> targetPositions;
    targetPositions.reserve(targetKeys.size());
    for (auto& key : targetKeys) {
        if (Enemy* e = enemies.Get(key))
            targetPositions.push_back(e->m_position);
    }

    Attack attack;
    attack.m_combat.m_targetKeys = targetKeys;
    BuildPayload(tower, attack.m_combat);
    attack.m_visual = BuildVisual(tower, std::move(targetPositions));
    attack.m_duration = attack.m_maxDuration = tower.m_presentation.m_attackDuration;

    if (tower.m_presentation.m_muzzleDesc.m_count > 0) // muzzle burst
        particles.Emit(tower.m_position, tower.m_presentation.m_muzzleDesc);

    sound.PlaySfx(tower.m_presentation.m_attackSound); // no-op when the key is empty

    attacks.push_back(std::move(attack));
}

AttackVisual TowerSystem::BuildVisual(const Tower& tower, std::vector<Vector2> targetPositions) {
    AttackVisual visual;
    visual.m_origin = tower.m_position;
    visual.m_targetPositions = std::move(targetPositions);
    visual.m_radius = tower.GetAttack()->m_liveRange;
    visual.m_color = tower.m_presentation.m_color;
    visual.m_style = tower.m_presentation.m_style;
    return visual;
}

std::vector<DenseSlotMap<Enemy>::Key> TowerSystem::FindTargets(const Tower& tower, DenseSlotMap<Enemy>& enemies, int maxTargets) {
    std::vector<Enemy*> inRange = FindEnemiesInRange(tower, enemies);

    std::sort(inRange.begin(), inRange.end(), [&](const Enemy* a, const Enemy* b) {
        return CompareTarget(*a, *b, tower.GetAttack()->m_targetingMode, tower.m_position);
    });

    int size = static_cast<int>(inRange.size());
    int count = (maxTargets == 0) ? size : std::min(size, maxTargets); // 0 = target all in range

    std::vector<DenseSlotMap<Enemy>::Key> result;
    result.reserve(count);
    for (int i = 0; i < count; i++)
        result.push_back(enemies.KeyOf(inRange[i]));

    return result;
}

std::vector<Enemy*> TowerSystem::FindEnemiesInRange(const Tower& tower, DenseSlotMap<Enemy>& enemies) {
    float range = tower.GetAttack()->m_liveRange;
    std::vector<Enemy*> result;
    for (auto& enemy : enemies) {
        if (enemy.m_currentHealth <= 0.0f) continue;
        if (range >= Vector2Distance(enemy.m_position, tower.m_position))
            result.push_back(&enemy);
    }
    return result;
}

void TowerSystem::BuildPayload(const Tower& tower, AttackPayload& payload) {
    // Damage comes from the AttackModule; armor pierce and behavioural effects come from modules
    payload.m_damage = tower.GetAttack()->m_liveDamage;
    if (tower.m_presentation.m_impactDesc.m_count > 0)
        payload.m_impactDescs.push_back(tower.m_presentation.m_impactDesc);
    if (tower.m_presentation.m_critImpactDesc.m_count > 0)
        payload.m_critImpactDescs.push_back(tower.m_presentation.m_critImpactDesc);
    for (auto& mod : tower.m_modules)
        mod->Contribute(payload);
}

// Base damage for one hit: effective armor reduction plus a single crit roll.
// outCrit reports whether this hit crit (used for crit-only impact particles).
static float ResolveDamage(const AttackPayload& payload, const Enemy& enemy, bool& outCrit) {
    float armor = std::max(0.0f, enemy.GetBaseStats()->m_liveArmor - payload.m_armorPierce);
    float dmg = payload.m_damage;
    outCrit = payload.m_critChance > 0.0f
        && GetRandomValue(0, 99) < (int)(payload.m_critChance * 100.0f);
    if (outCrit)
        dmg *= payload.m_critMultiplier;
    // Armor never fully nullifies a hit: when it meets or exceeds the attack, the hit still chips
    // min(attack, 1.0) instead of dropping to 0 (and so can never heal the enemy).
    float net = dmg - armor;
    return (net > 0.0f) ? net : std::min(dmg, 1.0f);
}

// A pre-existing Weakness adds flat bonus damage to this hit, then is consumed (0 if none).
static float ConsumeWeaknessBonus(Enemy& enemy) {
    if (Effect* w = enemy.FindEffect(EffectType::Weakness)) {
        float bonus = w->m_value;
        enemy.RemoveEffect(EffectType::Weakness);
        return bonus;
    }
    return 0.0f;
}

// Post-damage effect bookkeeping: any hit wakes a stunned enemy, then the payload's own effects
// are applied last so a weakness/stun tower never consumes the effect it just applied.
static void ApplyOnHitEffects(const AttackPayload& payload, Enemy& enemy) {
    enemy.RemoveEffect(EffectType::Stun);
    for (auto& effect : payload.m_effects)
        enemy.AddEffect(effect);
}

// Impact particles — all modules contribute their burst; crit adds extra descs.
static void EmitImpact(ParticleSystem& particles, const Enemy& enemy, bool crit, const AttackPayload& payload) {
    for (auto& desc : payload.m_impactDescs)
        particles.Emit(enemy.m_position, desc);
    if (crit) {
        for (auto& desc : payload.m_critImpactDescs)
            particles.Emit(enemy.m_position, desc);
    }
}

// Resolve combat once on the fire frame, then decay the shared lifetime that drives the visual
// fade. An attack is erased only once it has resolved and its lifetime has fully drained, so an
// instant (zero-duration) attack still lands its damage before being culled.
void TowerSystem::TickAttacks(float dt, DenseSlotMap<Enemy>& enemies, std::vector<Attack>& attacks, ParticleSystem& particles) {
    for (auto& attack : attacks) {
        AttackPayload& payload = attack.m_combat;
        if (!payload.m_resolved) {
            for (auto& key : payload.m_targetKeys) {
                Enemy* enemy = enemies.Get(key);
                if (!enemy) continue;

                bool crit = false;
                float net = ResolveDamage(payload, *enemy, crit);
                net += ConsumeWeaknessBonus(*enemy); // before interception/health, per effect rules
                // Each module gets a shot at the incoming damage, in declaration order: shields absorb,
                // resistance scales, evasion/barrier can negate it outright. ShieldModule is just one
                // of these (no longer special-cased).
                for (auto& mod : enemy->m_modules)
                    net = mod->InterceptDamage(net);
                enemy->m_currentHealth -= net;
                ApplyOnHitEffects(payload, *enemy); // stun cleared after damage; new effects applied last
                EmitImpact(particles, *enemy, crit, payload);
            }
            payload.m_resolved = true;
        }
        attack.m_duration -= dt;
    }
    std::erase_if(attacks, [](const Attack& a) {
        return a.m_combat.m_resolved && a.m_duration <= 0.0f;
    });
}

static float TotalShield(const Enemy& enemy) {
    // Sum every module's shield contribution (ShieldModule and ShieldRegenModule both report one) so
    // MostShield targeting accounts for all shield sources, not just the cached ShieldModule.
    float total = 0.0f;
    for (const auto& mod : enemy.m_modules)
        total += mod->GetShield();
    return total;
}

// Health plus all shields: the real damage needed to kill, used by the Weakest/Toughest modes so they
// account for shielded enemies (raw current health alone ignores the shield pool).
static float EffectiveHealth(const Enemy& enemy) {
    return enemy.m_currentHealth + TotalShield(enemy);
}

bool TowerSystem::CompareTarget(const Enemy& a, const Enemy& b, TargetingMode mode, Vector2 towerPos) {
    switch (mode) {
        case TargetingMode::First:          return a.m_progress < b.m_progress;
        case TargetingMode::Last:           return a.m_progress > b.m_progress;
        case TargetingMode::MostHealth:     return a.m_currentHealth > b.m_currentHealth;
        case TargetingMode::LowestHealth:   return a.m_currentHealth < b.m_currentHealth;
        case TargetingMode::Fastest:        return a.GetBaseStats()->m_liveSpeed > b.GetBaseStats()->m_liveSpeed;
        case TargetingMode::Slowest:        return a.GetBaseStats()->m_liveSpeed < b.GetBaseStats()->m_liveSpeed;
        case TargetingMode::MostArmor:      return a.GetBaseStats()->m_liveArmor > b.GetBaseStats()->m_liveArmor;
        case TargetingMode::MostShield:     return TotalShield(a) > TotalShield(b);
        case TargetingMode::LowestEffectiveHealth: return EffectiveHealth(a) < EffectiveHealth(b);
        case TargetingMode::MostEffectiveHealth:   return EffectiveHealth(a) > EffectiveHealth(b);
        case TargetingMode::MostMaxHealth:  return a.GetBaseStats()->m_maxHealth > b.GetBaseStats()->m_maxHealth;
        case TargetingMode::MostReward:     return a.GetBaseStats()->m_reward > b.GetBaseStats()->m_reward;
        case TargetingMode::MostLives:      return a.GetBaseStats()->m_livesOnReach > b.GetBaseStats()->m_livesOnReach;
        // Squared distance: same ordering as distance, no sqrt.
        case TargetingMode::Closest:        return Vector2DistanceSqr(a.m_position, towerPos) < Vector2DistanceSqr(b.m_position, towerPos);
        case TargetingMode::Farthest:       return Vector2DistanceSqr(a.m_position, towerPos) > Vector2DistanceSqr(b.m_position, towerPos);
    }
    return false;
}
