#pragma once

#include <string>
#include <vector>
#include <utility>
#include <unordered_map>
#include <cstdio>
#include <world/module_def.hpp>
#include <world/desc_line.hpp>

// Upgrades for towers and enemies. Both share a common payload (scalar deltas + added modules)
// and a key-formatting policy; tower upgrades add a purchasable cost, enemy upgrades don't.

// --- Shared formatting + payload ---------------------------------------------

// A stat-key formatting policy: maps a raw upgrade key to a readable label and decides how
// additive deltas render — plain ("+2 Range"), fraction ("+10% Crit", value stored in [0..1]),
// or whole-percent ("+20% Slow", value already a percent). Tower and enemy upgrades share the
// Describe logic below and differ only in the policy they supply.
struct StatFormat {
    const char* (*label)(const std::string& key);
    bool (*isFraction)(const std::string& key);
    bool (*isPercent)(const std::string& key);
};

// Predicate for policies with no fraction/percent keys (e.g. enemy upgrades).
inline bool NoSpecialKey(const std::string&) { return false; }

// One additive delta line, formatted per the policy.
inline std::string FormatAddDelta(const StatFormat& fmt, const std::string& key, float v) {
    char buf[48];
    if (fmt.isFraction(key))     snprintf(buf, sizeof(buf), "+%g%% %s", v * 100.0f, fmt.label(key));
    else if (fmt.isPercent(key)) snprintf(buf, sizeof(buf), "+%g%% %s", v, fmt.label(key));
    else                         snprintf(buf, sizeof(buf), "+%g %s", v, fmt.label(key));
    return buf;
}

// One multiplicative delta line: "x1.5 Rate".
inline std::string FormatMulDelta(const StatFormat& fmt, const std::string& key, float v) {
    char buf[48];
    snprintf(buf, sizeof(buf), "x%g %s", v, fmt.label(key));
    return buf;
}

// Shared payload of one upgrade step: scalar deltas (additive + multiplicative) broadcast to
// the entity's modules, plus any new modules to append. Tower upgrades extend this with a
// player-facing cost; enemy upgrades use it as-is (applied by wave tier, not purchased).
struct UpgradeDef {
    std::vector<std::pair<std::string, float>> m_adds; // stat key -> additive delta
    std::vector<std::pair<std::string, float>> m_muls; // stat key -> multiplicative factor
    std::vector<ModuleDef> m_addModules;               // new modules built via the factory

    // Append this step's deltas as display lines, mirroring a module's DescribeStats.
    void Describe(std::vector<DescLine>& out, const StatFormat& fmt) const {
        for (const auto& [key, v] : m_adds)
            out.push_back({FormatAddDelta(fmt, key, v), RAYWHITE});
        for (const auto& [key, v] : m_muls)
            out.push_back({FormatMulDelta(fmt, key, v), RAYWHITE});
        for (const auto& mod : m_addModules)
            if (!mod.m_type.empty()) out.push_back({"Adds " + mod.m_type, RAYWHITE});
    }
};

// Parse the shared add/mul/modules fields of an upgrade step from its TOML table. The
// (tower-only) cost field is read by the caller.
inline void ParseUpgradeFields(const toml::table& j, UpgradeDef& up) {
    if (auto add = j["add"].as_table())
        for (auto&& [k, v] : *add) up.m_adds.push_back({std::string(k.str()), v.value_or(0.0f)});
    if (auto mul = j["mul"].as_table())
        for (auto&& [k, v] : *mul) up.m_muls.push_back({std::string(k.str()), v.value_or(0.0f)});
    if (auto mods = j["modules"].as_array())
        for (auto&& m : *mods)
            if (auto mt = m.as_table()) up.m_addModules.push_back(MakeModuleDef(*mt));
}

// --- Tower upgrades ----------------------------------------------------------

// Readable name for an upgrade key (combat stats and module parameters); falls back to the raw key.
inline const char* StatLabel(const std::string& key) {
    static const std::unordered_map<std::string, const char*> labels = {
        {"damage", "Damage"},          {"shotsPerMinute", "Rate"},       {"range", "Range"},
        {"targetCount", "Targets"},    {"armorPierce", "Pierce"},        {"critChance", "Crit"},
        {"critMultiplier", "Crit Mult"}, {"slowPercent", "Slow"},        {"slowDuration", "Slow Time"},
        {"burnDamage", "Burn"},        {"burnDuration", "Burn Time"},    {"shredAmount", "Shred"},
        {"shredDuration", "Shred Time"}, {"weaknessAmount", "Weakness"}, {"weaknessDuration", "Weak Time"},
        {"stunDuration", "Stun Time"}, {"bonusPerStack", "Ramp"},        {"maxStacks", "Max Stacks"},
        {"idleTime", "Idle"},
    };
    auto it = labels.find(key);
    return it != labels.end() ? it->second : key.c_str();
}

// Keys stored as a fraction in [0..1]: render the value scaled to a percentage (0.1 -> "10%").
inline bool IsFractionKey(const std::string& key) {
    return key == "critChance";
}

// Keys already stored as a whole percentage: render with "%" but without scaling (20 -> "20%").
inline bool IsPercentKey(const std::string& key) {
    return key == "slowPercent";
}

// Formatting policy for tower upgrade lines.
inline const StatFormat& TowerStatFormat() {
    static const StatFormat fmt{StatLabel, IsFractionKey, IsPercentKey};
    return fmt;
}

// One purchasable upgrade level for a tower: the shared scalar deltas / added modules plus a cost.
struct TowerUpgrade : UpgradeDef {
    int m_cost = 0;

    void Describe(std::vector<DescLine>& out) const { UpgradeDef::Describe(out, TowerStatFormat()); }
};

// --- Enemy upgrades ----------------------------------------------------------

// Readable name for an enemy upgrade key (base stats and module parameters); falls back to the raw key.
inline const char* EnemyStatLabel(const std::string& key) {
    static const std::unordered_map<std::string, const char*> labels = {
        {"maxHealth", "Health"}, {"speed", "Speed"},      {"reward", "Reward"},
        {"armor", "Armor"},      {"shield", "Shield"},    {"regenRate", "Regen"},
        {"splitCount", "Split"},
    };
    auto it = labels.find(key);
    return it != labels.end() ? it->second : key.c_str();
}

// Formatting policy for enemy upgrade lines (no fraction/percent keys).
inline const StatFormat& EnemyStatFormat() {
    static const StatFormat fmt{EnemyStatLabel, NoSpecialKey, NoSpecialKey};
    return fmt;
}

// One upgrade step for an enemy: the shared scalar deltas / added modules. Mirrors TowerUpgrade,
// minus the player-facing cost (enemy upgrades are applied by wave tier, not purchased).
struct EnemyUpgrade : UpgradeDef {
    void Describe(std::vector<DescLine>& out) const { UpgradeDef::Describe(out, EnemyStatFormat()); }
};
