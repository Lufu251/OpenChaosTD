#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <functional>
#include <memory>
#include <optional>
#include <expected>
#include <toml++/toml.hpp>
#include <world/module_def.hpp>
#include <world/enemy.hpp>
#include <engine/util/file_store.hpp>
#include <content/emitter_presets.hpp>

class EnemyFactory {
public:
    // dataDir is the active datapack's data directory (relative to the project
    // root), e.g. "datapacks/default/data".
    void Load(FileStore& fileStore, const EmitterPresets& presets, const std::string& dataDir);
    // Drop all loaded templates (called when the active datapack is unloaded).
    void Clear();
    // Build an enemy by template name. Returns an error message when the name is unknown
    // (e.g. a stale save or edited datapack) rather than crashing the caller.
    std::expected<Enemy, std::string> Create(const std::string& name) const;
    bool Has(const std::string& name) const;

    // Build a single module from its definition (also used to append upgrade modules).
    std::unique_ptr<EnemyModule> BuildModule(const ModuleDef& mod) const;

    // Apply one upgrade step to an enemy: broadcast deltas via Enemy::PatchStats and, when
    // includeModules is set, append any new modules. The enemy analogue of
    // PlayingState::UpgradeSelectedTower (no gold/UI). Scalars stack per tier; modules append once.
    void ApplyUpgrade(Enemy& enemy, const EnemyUpgrade& up, bool includeModules = true) const;

    // Apply the enemy's single upgrade definition `tier` times, stacking the scalar deltas once per
    // tier and appending its modules only on the first tier. The wave manager scales its spawn
    // prototypes with this; the world layer uses it to scale Split/Summoner children to the wave tier.
    // No-op when tier <= 0 or the enemy defines no upgrade.
    void ApplyTierUpgrades(Enemy& enemy, int tier) const;

private:
    using ModuleBuilder = std::function<std::unique_ptr<EnemyModule>(const ModuleDef&)>;

    struct EnemyTemplate {
        std::string name;
        std::string description;
        float maxHealth = 10.0f;
        float speed = 50.0f;
        int reward = 5;
        int livesOnReach = 1;
        EnemyPresentation presentation;
        std::vector<ModuleDef> modules;
        std::optional<EnemyUpgrade> upgrade; // single upgrade option, applied once per upgrade tier
    };

    std::unordered_map<std::string, ModuleBuilder> m_builders;
    std::unordered_map<std::string, EnemyTemplate> m_templates;
};
