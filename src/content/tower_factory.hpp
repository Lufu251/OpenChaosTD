#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <functional>
#include <memory>
#include <expected>
#include <toml++/toml.hpp>
#include <world/module_def.hpp>
#include <world/tower.hpp>
#include <engine/util/file_store.hpp>
#include <content/emitter_presets.hpp>

class TowerFactory {
public:
    // dataDir is the active datapack's data directory (relative to the project
    // root), e.g. "datapacks/default/data".
    void Load(FileStore& fileStore, const EmitterPresets& presets, const std::string& dataDir);
    // Drop all loaded templates (called when the active datapack is unloaded).
    void Clear();
    // Build a tower by template name. Returns an error message when the name is unknown
    // (e.g. a stale save or edited datapack) rather than crashing the caller.
    std::expected<Tower, std::string> Create(const std::string& name) const;
    bool Has(const std::string& name) const;

    const std::vector<std::string>& GetNames() const { return m_order; }
    int GetCost(const std::string& name) const;
    float GetRange(const std::string& name) const;
    const std::string& GetTexture(const std::string& name) const;

    // Build one effect module from a module definition (shared by Create and upgrades).
    std::unique_ptr<TowerModule> BuildModule(const ModuleDef& mod) const;

    // Apply one upgrade tier's stat deltas and new modules to a tower. Purely mechanical:
    // it does NOT touch gold, m_cost or m_level — callers own that policy. Shared by the
    // live upgrade path (PlayingState) and the save loader (which replays tiers 0..level-1).
    void ApplyUpgradeStats(Tower& tower, const TowerUpgrade& up) const;

private:
    using ModuleBuilder = std::function<std::unique_ptr<TowerModule>(const ModuleDef&)>;

    // Look up the optional "effect" emitter preset on a module definition (default if absent).
    EmitterDesc ResolveEmitter(const toml::table& j) const;

    struct TowerTemplate {
        std::string name;
        std::string description;
        int cost = 100;
        float range = 0.0f; // cached from the Attack module def for GetRange (build preview/ghost)
        TowerPresentation presentation;
        std::vector<ModuleDef> modules; // all module defs (Attack/Passive/effects), built per Create
        std::vector<TowerUpgrade> upgrades;
    };

    const EmitterPresets* m_presets = nullptr;
    std::unordered_map<std::string, ModuleBuilder> m_builders;
    std::unordered_map<std::string, TowerTemplate> m_templates;
    std::vector<std::string> m_order;
};
