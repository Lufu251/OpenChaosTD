#include <world/modules.hpp>
#include <world/enemy.hpp> // RegenerationModule::Tick reads Enemy stats/health
#include <algorithm>

// ============================================================================
// Tower modules
// ============================================================================

// --- AttackModule ---

void AttackModule::ResetLive() {
    m_liveDamage = m_damage;
    m_liveShotsPerMinute = m_shotsPerMinute;
    m_liveRange = m_range;
    m_liveTargetCount = m_targetCount;
}

void AttackModule::PatchStats(const std::string& key, float v, bool mul) {
    if      (key == "damage")         ApplyDelta(m_damage, v, mul);
    else if (key == "shotsPerMinute") ApplyDelta(m_shotsPerMinute, v, mul);
    else if (key == "range")          ApplyDelta(m_range, v, mul);
    else if (key == "targetCount")    PatchInt(m_targetCount, v, mul);
}

void AttackModule::DescribeStats(std::vector<DescLine>& out) const {
    PushStatLine(out, RAYWHITE, "Damage:  %g", m_liveDamage);
    PushStatLine(out, RAYWHITE, "Range:   %.0f", m_liveRange);
    PushStatLine(out, RAYWHITE, "Rate:    %d/min", static_cast<int>(m_liveShotsPerMinute + 0.5f));
    PushStatLine(out, RAYWHITE, "Targets: %d", m_liveTargetCount);
}

// --- ArmorPierceModule ---

ArmorPierceModule::ArmorPierceModule(float amount)
    : m_amount(amount) {}

void ArmorPierceModule::Contribute(AttackPayload& attack) const {
    attack.m_armorPierce += m_amount;
}

void ArmorPierceModule::DescribeStats(std::vector<DescLine>& out) const {
    PushStatLine(out, GOLD, "Pierce:  %g", m_amount);
}

void ArmorPierceModule::PatchStats(const std::string& key, float v, bool mul) {
    if (key == "armorPierce") ApplyDelta(m_amount, v, mul);
}

// --- SlowModule ---

SlowModule::SlowModule(float slowPercent, float duration, EmitterDesc particleDesc)
    : m_slowPercent(slowPercent), m_duration(duration), m_particleDesc(std::move(particleDesc)) {}

void SlowModule::Contribute(AttackPayload& attack) const {
    Effect e(EffectType::Slow, m_duration, m_slowPercent);
    e.m_particleDesc = m_particleDesc;
    attack.m_effects.push_back(std::move(e));
}

void SlowModule::DescribeStats(std::vector<DescLine>& out) const {
    PushStatLine(out, SKYBLUE, "Slow:    %.0f%%  %.1fs", m_slowPercent, m_duration);
}

void SlowModule::PatchStats(const std::string& key, float v, bool mul) {
    if      (key == "slowPercent")  ApplyDelta(m_slowPercent, v, mul);
    else if (key == "slowDuration") ApplyDelta(m_duration, v, mul);
}

// --- BurnModule ---

BurnModule::BurnModule(float value, float duration, EmitterDesc particleDesc)
    : m_damage(value), m_duration(duration), m_particleDesc(std::move(particleDesc)) {}

void BurnModule::Contribute(AttackPayload& attack) const {
    Effect e(EffectType::Burn, m_duration, m_damage);
    e.m_particleDesc = m_particleDesc;
    attack.m_effects.push_back(std::move(e));
}

void BurnModule::DescribeStats(std::vector<DescLine>& out) const {
    PushStatLine(out, ORANGE, "Burn:    %g/s  %.1fs", m_damage, m_duration);
}

void BurnModule::PatchStats(const std::string& key, float v, bool mul) {
    if      (key == "burnDamage")   ApplyDelta(m_damage, v, mul);
    else if (key == "burnDuration") ApplyDelta(m_duration, v, mul);
}

// --- ArmorShredModule ---

ArmorShredModule::ArmorShredModule(float amount, float duration, EmitterDesc particleDesc)
    : m_amount(amount), m_duration(duration), m_particleDesc(std::move(particleDesc)) {}

void ArmorShredModule::Contribute(AttackPayload& attack) const {
    Effect e(EffectType::ArmorShred, m_duration, m_amount);
    e.m_particleDesc = m_particleDesc;
    attack.m_effects.push_back(std::move(e));
}

void ArmorShredModule::DescribeStats(std::vector<DescLine>& out) const {
    PushStatLine(out, GRAY, "Shred:   %g  %.1fs", m_amount, m_duration);
}

void ArmorShredModule::PatchStats(const std::string& key, float v, bool mul) {
    if      (key == "shredAmount")   ApplyDelta(m_amount, v, mul);
    else if (key == "shredDuration") ApplyDelta(m_duration, v, mul);
}

// --- WeaknessModule ---

WeaknessModule::WeaknessModule(float amount, float duration, EmitterDesc particleDesc)
    : m_amount(amount), m_duration(duration), m_particleDesc(std::move(particleDesc)) {}

void WeaknessModule::Contribute(AttackPayload& attack) const {
    Effect e(EffectType::Weakness, m_duration, m_amount);
    e.m_particleDesc = m_particleDesc;
    attack.m_effects.push_back(std::move(e));
}

void WeaknessModule::DescribeStats(std::vector<DescLine>& out) const {
    PushStatLine(out, PURPLE, "Weak:    +%g  %.1fs", m_amount, m_duration);
}

void WeaknessModule::PatchStats(const std::string& key, float v, bool mul) {
    if      (key == "weaknessAmount")   ApplyDelta(m_amount, v, mul);
    else if (key == "weaknessDuration") ApplyDelta(m_duration, v, mul);
}

// --- StunModule ---

StunModule::StunModule(float duration, EmitterDesc particleDesc)
    : m_duration(duration), m_particleDesc(std::move(particleDesc)) {}

void StunModule::Contribute(AttackPayload& attack) const {
    // m_value = duration so the "equal or stronger" rule means a longer stun wins
    Effect e(EffectType::Stun, m_duration, m_duration);
    e.m_particleDesc = m_particleDesc;
    attack.m_effects.push_back(std::move(e));
}

void StunModule::DescribeStats(std::vector<DescLine>& out) const {
    PushStatLine(out, YELLOW, "Stun:    %.1fs", m_duration);
}

void StunModule::PatchStats(const std::string& key, float v, bool mul) {
    if (key == "stunDuration") ApplyDelta(m_duration, v, mul);
}

// --- CritModule ---

CritModule::CritModule(float critChance, float critMultiplier)
    : m_critChance(critChance), m_critMultiplier(critMultiplier) {}

void CritModule::Contribute(AttackPayload& attack) const {
    attack.m_critChance = m_critChance;
    attack.m_critMultiplier = m_critMultiplier;
}

void CritModule::DescribeStats(std::vector<DescLine>& out) const {
    PushStatLine(out, YELLOW, "Crit:    %.0f%%  x%.1f", m_critChance * 100.0f, m_critMultiplier);
}

void CritModule::PatchStats(const std::string& key, float v, bool mul) {
    if      (key == "critChance")     ApplyDelta(m_critChance, v, mul);
    else if (key == "critMultiplier") ApplyDelta(m_critMultiplier, v, mul);
}

// --- RampUpModule ---

RampUpModule::RampUpModule(float bonusPerStack, int maxStacks, float idleTime)
    : m_bonusPerStack(bonusPerStack), m_idleTime(idleTime), m_maxStacks(maxStacks) {}

void RampUpModule::ContributeTower(AttackModule& attack) const {
    attack.m_liveShotsPerMinute += m_stacks * m_bonusPerStack;
}

void RampUpModule::Tick(float dt) {
    if (m_stacks <= 0) return;
    m_idleTimer += dt;
    if (m_idleTimer >= m_idleTime) { // idle too long — lose the ramp
        m_stacks = 0;
        m_idleTimer = 0.0f;
    }
}

void RampUpModule::OnFire() {
    if (m_stacks < m_maxStacks) m_stacks++;
    m_idleTimer = 0.0f;
}

void RampUpModule::DescribeStats(std::vector<DescLine>& out) const {
    PushStatLine(out, GREEN, "Ramp:    %d/%d", m_stacks, m_maxStacks);
}

void RampUpModule::PatchStats(const std::string& key, float v, bool mul) {
    if      (key == "bonusPerStack") ApplyDelta(m_bonusPerStack, v, mul);
    else if (key == "idleTime")      ApplyDelta(m_idleTime, v, mul);
    else if (key == "maxStacks")     m_maxStacks = mul ? static_cast<int>(m_maxStacks * v + 0.5f) : m_maxStacks + static_cast<int>(v + 0.5f);
}

// ============================================================================
// Enemy modules
// ============================================================================

// --- BaseStatsModule ---

void BaseStatsModule::ResetLive() {
    m_liveSpeed = m_speed;
    m_liveArmor = 0.0f; // no innate armor; ArmorModule contributes it via ContributeStats
}

void BaseStatsModule::PatchStats(const std::string& key, float v, bool mul) {
    if      (key == "maxHealth") ApplyDelta(m_maxHealth, v, mul);
    else if (key == "speed")     ApplyDelta(m_speed, v, mul);
    else if (key == "reward")       PatchInt(m_reward, v, mul);
    else if (key == "livesOnReach") PatchInt(m_livesOnReach, v, mul);
}

void BaseStatsModule::DescribeStats(std::vector<DescLine>& out) const {
    PushStatLine(out, RAYWHITE, "Health:  %g", m_maxHealth);
    PushStatLine(out, RAYWHITE, "Speed:   %g", m_liveSpeed);
}

// --- RegenerationModule ---

void RegenerationModule::Tick(float dt, Enemy& enemy, std::vector<SpawnRequest>& /*outSpawns*/) {
    float maxHealth = enemy.GetBaseStats()->m_maxHealth;
    enemy.m_currentHealth = std::min(maxHealth, enemy.m_currentHealth + m_rate * dt);
}

void RegenerationModule::DescribeStats(std::vector<DescLine>& out) const {
    PushStatLine(out, RAYWHITE, "Regen:   %g/s", m_rate);
}

void RegenerationModule::PatchStats(const std::string& key, float v, bool mul) {
    if (key == "regenRate") ApplyDelta(m_rate, v, mul);
}

// --- ArmorModule ---

void ArmorModule::ContributeStats(BaseStatsModule& base) const {
    base.m_liveArmor += m_amount;
}

void ArmorModule::DescribeStats(std::vector<DescLine>& out) const {
    PushStatLine(out, RAYWHITE, "Armor:   %g", m_amount);
}

void ArmorModule::PatchStats(const std::string& key, float v, bool mul) {
    if (key == "armor") ApplyDelta(m_amount, v, mul);
}

// --- ImmuneModule ---

bool ImmuneModule::ShouldBlock(EffectType type) const {
    return m_effect == type;
}

void ImmuneModule::DescribeStats(std::vector<DescLine>& out) const {
    switch (m_effect) {
        case EffectType::Slow:       out.push_back({"Immune:  Slow",  SKYBLUE}); return;
        case EffectType::Burn:       out.push_back({"Immune:  Burn",  ORANGE});  return;
        case EffectType::ArmorShred: out.push_back({"Immune:  Shred", GRAY});    return;
        case EffectType::Stun:       out.push_back({"Immune:  Stun",  YELLOW});  return;
        case EffectType::Weakness:   out.push_back({"Immune:  Weak",  PURPLE});  return;
    }
}

// --- ShieldModule ---

float ShieldModule::InterceptDamage(float incoming) {
    if (m_currentShield <= 0.0f) return incoming;
    float absorbed = std::min(m_currentShield, incoming);
    m_currentShield -= absorbed;
    return incoming - absorbed;
}

void ShieldModule::DescribeStats(std::vector<DescLine>& out) const {
    PushStatLine(out, {0, 220, 255, 255}, "Shield:  %.0f/%.0f", m_currentShield, m_maxShield);
}

void ShieldModule::PatchStats(const std::string& key, float v, bool mul) {
    // Scale the pool and refill the live shield so a buffed shield is also topped up.
    if (key == "shield") {
        ApplyDelta(m_maxShield, v, mul);
        m_currentShield = m_maxShield;
    }
}

// --- SplitModule ---

std::optional<SpawnRequest> SplitModule::OnDeath() const {
    return SpawnRequest{m_childType, m_count, m_spacing};
}

void SplitModule::DescribeStats(std::vector<DescLine>& out) const {
    PushStatLine(out, RAYWHITE, "Split:   %dx %s", m_count, m_childType.c_str());
}

void SplitModule::PatchStats(const std::string& key, float v, bool mul) {
    if (key == "splitCount") PatchInt(m_count, v, mul);
}

// --- ResistanceModule ---

float ResistanceModule::InterceptDamage(float incoming) {
    return incoming * (1.0f - m_percent / 100.0f);
}

void ResistanceModule::DescribeStats(std::vector<DescLine>& out) const {
    PushStatLine(out, SKYBLUE, "Resist:  %.0f%%", m_percent);
}

void ResistanceModule::PatchStats(const std::string& key, float v, bool mul) {
    if (key == "resistPercent") ApplyDelta(m_percent, v, mul);
}

// --- EvasionModule ---

float EvasionModule::InterceptDamage(float incoming) {
    // Same global RNG stream as the tower crit roll (tower_system.cpp), so a save replay is stable.
    // Roll on a 0..9999 scale so sub-1% dodge chances aren't truncated to 0% (a 0.5% chance fires).
    bool dodged = GetRandomValue(0, 9999) < static_cast<int>(m_dodgeChance * 10000.0f);
    return dodged ? 0.0f : incoming;
}

void EvasionModule::DescribeStats(std::vector<DescLine>& out) const {
    PushStatLine(out, SKYBLUE, "Dodge:   %.0f%%", m_dodgeChance * 100.0f);
}

void EvasionModule::PatchStats(const std::string& key, float v, bool mul) {
    if (key == "dodgeChance") ApplyDelta(m_dodgeChance, v, mul);
}

// --- BarrierModule ---

float BarrierModule::InterceptDamage(float incoming) {
    if (m_hitsLeft <= 0) return incoming;
    m_hitsLeft--; // one charge fully eats this hit, regardless of magnitude
    return 0.0f;
}

void BarrierModule::DescribeStats(std::vector<DescLine>& out) const {
    PushStatLine(out, {0, 220, 255, 255}, "Barrier: %d/%d", m_hitsLeft, m_maxHits);
}

void BarrierModule::PatchStats(const std::string& key, float v, bool mul) {
    // Scale the charge count and refill, mirroring ShieldModule topping up its pool on upgrade.
    if (key == "hitCount") {
        PatchInt(m_maxHits, v, mul);
        m_hitsLeft = m_maxHits;
    }
}

// --- EnrageModule ---

void EnrageModule::Tick(float /*dt*/, Enemy& enemy, std::vector<SpawnRequest>& /*outSpawns*/) {
    float maxHealth = enemy.GetBaseStats()->m_maxHealth;
    m_active = maxHealth > 0.0f && enemy.m_currentHealth <= maxHealth * m_healthThreshold;
}

void EnrageModule::ContributeStats(BaseStatsModule& base) const {
    if (m_active) base.m_liveSpeed += m_speedBonus;
}

void EnrageModule::DescribeStats(std::vector<DescLine>& out) const {
    PushStatLine(out, RED, "Enrage:  +%g @ %.0f%%", m_speedBonus, m_healthThreshold * 100.0f);
}

void EnrageModule::PatchStats(const std::string& key, float v, bool mul) {
    if      (key == "healthThreshold") ApplyDelta(m_healthThreshold, v, mul);
    else if (key == "speedBonus")      ApplyDelta(m_speedBonus, v, mul);
}

// --- ShieldRegenModule ---

float ShieldRegenModule::InterceptDamage(float incoming) {
    m_timeSinceHit = 0.0f; // any hit restarts the recharge delay
    if (m_currentShield <= 0.0f) return incoming;
    float absorbed = std::min(m_currentShield, incoming);
    m_currentShield -= absorbed;
    return incoming - absorbed;
}

void ShieldRegenModule::Tick(float dt, Enemy& /*enemy*/, std::vector<SpawnRequest>& /*outSpawns*/) {
    m_timeSinceHit += dt;
    if (m_timeSinceHit >= m_rechargeDelay)
        m_currentShield = std::min(m_maxShield, m_currentShield + m_rechargeRate * dt);
}

void ShieldRegenModule::DescribeStats(std::vector<DescLine>& out) const {
    PushStatLine(out, {0, 220, 255, 255}, "Shield:  %.0f/%.0f  +%g/s", m_currentShield, m_maxShield, m_rechargeRate);
}

void ShieldRegenModule::PatchStats(const std::string& key, float v, bool mul) {
    if      (key == "shield")        { ApplyDelta(m_maxShield, v, mul); m_currentShield = m_maxShield; }
    else if (key == "rechargeRate")  ApplyDelta(m_rechargeRate, v, mul);
    else if (key == "rechargeDelay") ApplyDelta(m_rechargeDelay, v, mul);
}

// --- AdrenalineModule ---

float AdrenalineModule::InterceptDamage(float incoming) {
    m_timer = m_duration; // taking a hit (re)arms the speed burst; damage passes through unchanged
    return incoming;
}

void AdrenalineModule::Tick(float dt, Enemy& /*enemy*/, std::vector<SpawnRequest>& /*outSpawns*/) {
    if (m_timer > 0.0f) m_timer -= dt;
}

void AdrenalineModule::ContributeStats(BaseStatsModule& base) const {
    if (m_timer > 0.0f) base.m_liveSpeed += m_speedBonus;
}

void AdrenalineModule::DescribeStats(std::vector<DescLine>& out) const {
    PushStatLine(out, RED, "Adrenal: +%g  %.1fs", m_speedBonus, m_duration);
}

void AdrenalineModule::PatchStats(const std::string& key, float v, bool mul) {
    if      (key == "speedBonus") ApplyDelta(m_speedBonus, v, mul);
    else if (key == "duration")   ApplyDelta(m_duration, v, mul);
}

// --- SummonerModule ---

void SummonerModule::Tick(float dt, Enemy& /*enemy*/, std::vector<SpawnRequest>& outSpawns) {
    if (m_interval <= 0.0f) return; // guard against a misconfigured interval (no spawning)
    m_timer += dt;
    while (m_timer >= m_interval) {
        m_timer -= m_interval;
        outSpawns.push_back(SpawnRequest{m_childType, m_count, m_spacing});
    }
}

void SummonerModule::DescribeStats(std::vector<DescLine>& out) const {
    PushStatLine(out, RAYWHITE, "Summon:  %dx %s / %.1fs", m_count, m_childType.c_str(), m_interval);
}

void SummonerModule::PatchStats(const std::string& key, float v, bool mul) {
    if      (key == "summonCount") PatchInt(m_count, v, mul);
    else if (key == "interval")    ApplyDelta(m_interval, v, mul);
}
