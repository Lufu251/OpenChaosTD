#include <content/enemy_factory.hpp>
#include <content/module_registry.hpp>
#include <world/modules.hpp>
#include <cassert>
#include <stdexcept>
#include <iostream>

static EffectType ParseEffectType(const std::string& name) {
    if (name == "Slow")       return EffectType::Slow;
    if (name == "ArmorShred") return EffectType::ArmorShred;
    if (name == "Stun")       return EffectType::Stun;
    if (name == "Weakness")   return EffectType::Weakness;
    return EffectType::Burn;
}

static EnemyPresentation ParsePresentation(const toml::table& j, const EmitterPresets& presets) {
    EnemyPresentation v;
    v.m_texture = j["texture"].value_or(std::string{});
    v.m_deathSound = j["deathSound"].value_or(std::string("enemy_death"));
    if (auto e = j["deathEmitter"].value<std::string>()) v.m_deathDescPtr = presets.GetPtr(*e);
    return v;
}

static EnemyUpgrade ParseUpgrade(const toml::table& j) {
    EnemyUpgrade up;
    ParseUpgradeFields(j, up); // shared add/mul/modules schema across towers and enemies
    return up;
}

void EnemyFactory::Clear() {
    m_templates.clear();
}

void EnemyFactory::Load(FileStore& fileStore, const EmitterPresets& presets, const std::string& dataDir) {
    Clear(); // replace any previously loaded pack's templates
    m_builders["Regeneration"] = [](const ModuleDef& def){ const toml::table& j = def.m_table; return std::make_unique<RegenerationModule>(j["regenRate"].value_or(0.0f)); };
    m_builders["Armor"]        = [](const ModuleDef& def){ const toml::table& j = def.m_table; return std::make_unique<ArmorModule>(j["armor"].value_or(0.0f)); };
    m_builders["Immune"]       = [](const ModuleDef& def){ const toml::table& j = def.m_table; return std::make_unique<ImmuneModule>(ParseEffectType(j["effect"].value_or(std::string{}))); };
    m_builders["Shield"]       = [](const ModuleDef& def){ const toml::table& j = def.m_table; return std::make_unique<ShieldModule>(j["shield"].value_or(0.0f)); };
    m_builders["Split"]        = [](const ModuleDef& def){ const toml::table& j = def.m_table; return std::make_unique<SplitModule>(j["child"].value_or(std::string{}), j["splitCount"].value_or(0), j["spacing"].value_or(12.0f)); };
    m_builders["Resistance"]   = [](const ModuleDef& def){ const toml::table& j = def.m_table; return std::make_unique<ResistanceModule>(j["resistPercent"].value_or(0.0f)); };
    m_builders["Evasion"]      = [](const ModuleDef& def){ const toml::table& j = def.m_table; return std::make_unique<EvasionModule>(j["dodgeChance"].value_or(0.0f)); };
    m_builders["Barrier"]      = [](const ModuleDef& def){ const toml::table& j = def.m_table; return std::make_unique<BarrierModule>(j["hitCount"].value_or(0)); };
    m_builders["Enrage"]       = [](const ModuleDef& def){ const toml::table& j = def.m_table; return std::make_unique<EnrageModule>(j["healthThreshold"].value_or(0.3f), j["speedBonus"].value_or(0.0f)); };
    m_builders["ShieldRegen"]  = [](const ModuleDef& def){ const toml::table& j = def.m_table; return std::make_unique<ShieldRegenModule>(j["shield"].value_or(0.0f), j["rechargeRate"].value_or(0.0f), j["rechargeDelay"].value_or(0.0f)); };
    m_builders["Adrenaline"]   = [](const ModuleDef& def){ const toml::table& j = def.m_table; return std::make_unique<AdrenalineModule>(j["speedBonus"].value_or(0.0f), j["duration"].value_or(0.0f)); };
    m_builders["Summoner"]     = [](const ModuleDef& def){ const toml::table& j = def.m_table; return std::make_unique<SummonerModule>(j["child"].value_or(std::string{}), j["summonCount"].value_or(1), j["spacing"].value_or(12.0f), j["interval"].value_or(5.0f)); };

    auto data = fileStore.LoadToml(dataDir + "/enemies.toml");
    auto enemies = data["enemies"].as_array();
    if (!enemies) {
        std::cerr << "EnemyFactory: failed to load enemies data\n";
        return;
    }

    for (auto&& entryNode : *enemies) {
        auto entry = entryNode.as_table();
        if (!entry) continue;

        EnemyTemplate tmpl;
        tmpl.name        = (*entry)["name"].value_or(std::string{});
        tmpl.description = (*entry)["description"].value_or(std::string{});
        tmpl.maxHealth   = (*entry)["maxHealth"].value_or(10.0f);
        tmpl.speed       = (*entry)["speed"].value_or(50.0f);
        tmpl.reward      = (*entry)["reward"].value_or(5);
        tmpl.livesOnReach = (*entry)["livesOnReach"].value_or(1);
        if (auto vis = (*entry)["presentation"].as_table()) tmpl.presentation = ParsePresentation(*vis, presets);

        if (auto mods = (*entry)["modules"].as_array())
            for (auto&& m : *mods)
                if (auto mod = m.as_table()) tmpl.modules.push_back(MakeModuleDef(*mod));

        if (auto up = (*entry)["upgrade"].as_table())
            tmpl.upgrade = ParseUpgrade(*up);

        std::string name = tmpl.name;
        m_templates[name] = std::move(tmpl);
        std::cout << "EnemyFactory: loaded '" << name << "'\n";
    }
}

std::expected<Enemy, std::string> EnemyFactory::Create(const std::string& name) const {
    auto it = m_templates.find(name);
    if (it == m_templates.end())
        return std::unexpected("EnemyFactory: unknown enemy '" + name + "'");

    const EnemyTemplate& tmpl = it->second;
    Enemy enemy;
    enemy.m_name        = tmpl.name;
    enemy.m_description = tmpl.description;
    enemy.m_presentation = tmpl.presentation;
    enemy.m_upgrade     = tmpl.upgrade ? &*tmpl.upgrade : nullptr; // stable: templates are fixed after Load

    // The core stats module is always present and added first, so it is cached (GetBaseStats) and
    // contributes its base speed before any other module. The analogue of the tower's AttackModule.
    auto base = std::make_unique<BaseStatsModule>();
    base->m_maxHealth    = tmpl.maxHealth;
    base->m_speed        = tmpl.speed;
    base->m_reward       = tmpl.reward;
    base->m_livesOnReach = tmpl.livesOnReach;
    enemy.AddModule(std::move(base));
    // BaseStatsModule must be the first module added so it owns the core stats and GetBaseStats()
    // caches it; everything below (and every appended upgrade module) relies on it already existing.
    assert(enemy.GetBaseStats() != nullptr && "BaseStatsModule must be added first");
    enemy.m_currentHealth = enemy.GetBaseStats()->m_maxHealth;

    // Build every other module; AddModule caches the ShieldModule.
    for (auto& mod : tmpl.modules)
        if (auto m = BuildModule(mod))
            enemy.AddModule(std::move(m));

    // Initial live-stat pass so the HUD/targeting read valid values before the first tick
    // (mirrors TowerFactory calling AttackModule::ResetLive at build time).
    enemy.RecomputeLive();

    return enemy;
}

std::unique_ptr<EnemyModule> EnemyFactory::BuildModule(const ModuleDef& mod) const {
    return BuildFromRegistry<std::unique_ptr<EnemyModule>>(m_builders, mod, "EnemyFactory");
}

void EnemyFactory::ApplyUpgrade(Enemy& enemy, const EnemyUpgrade& up, bool includeModules) const {
    // Each key routes through Enemy::PatchStats: base stats hit the enemy's own fields,
    // everything else is forwarded to the modules that recognize it.
    for (auto& [k, v] : up.m_adds) enemy.PatchStats(k, v, false);
    for (auto& [k, v] : up.m_muls) enemy.PatchStats(k, v, true);
    // Scalar deltas stack per tier, but added modules are appended only once across all tiers
    // (unlike tower upgrades, where each distinct level adds its modules once). The tier loop in
    // WaveManager::ApplyTierUpgrades passes includeModules=true on the first tier only.
    // Appended upgrade modules are always effect/aux modules (never a BaseStatsModule), so the
    // cached GetBaseStats() pointer set when the enemy was created stays valid and correct.
    if (includeModules)
        for (auto& mod : up.m_addModules)
            if (auto m = BuildModule(mod)) enemy.AddModule(std::move(m));
    enemy.m_level++;
}

void EnemyFactory::ApplyTierUpgrades(Enemy& enemy, int tier) const {
    if (tier <= 0 || !enemy.m_upgrade) return;

    // Re-apply the enemy's single upgrade option `tier` times so each tier stacks one more copy of the
    // same scalar deltas (scaling indefinitely in endless mode). Added modules are appended once (first
    // tier only), not per tier.
    for (int i = 0; i < tier; i++)
        ApplyUpgrade(enemy, *enemy.m_upgrade, i == 0);
}

bool EnemyFactory::Has(const std::string& name) const {
    return m_templates.count(name) > 0;
}
