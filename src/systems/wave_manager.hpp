#pragma once

#include <systems/world_system.hpp>
#include <content/enemy_factory.hpp>
#include <engine/util/file_store.hpp>
#include <random>
#include <string>
#include <vector>
#include <unordered_map>

struct GameData;

class WaveManager {
public:
    // How a wave's threat budget scales with the wave number (selected by the "type" field in the
    // budget block of waves.toml).
    enum class BudgetType {
        Exponential, // base_budget * (1 + growth_rate)^(wave - 1)
        StepLinear   // base_budget + linear_step * wave - tier_adjustment * tier, clamped to >= base
    };

    // A group of enemies sharing the same type and spawn timing within a wave
    struct SpawnGroup {
        std::string m_enemyType;
        int m_count = 1;
        int m_nest = -1;       // -1 = round-robin across all nests
        float m_interval = 1.0f; // seconds between enemies in this group
        float m_delay = 0.0f;    // offset from wave start before first enemy
    };

    struct WaveDef {
        std::vector<SpawnGroup> m_groups;
    };

    // dataDir is the active datapack's data directory (relative to the project root).
    void Load(FileStore& fileStore, const EnemyFactory& enemyFactory, const std::string& dataDir);

    // Advance wave state: tick timer, process spawn queue, detect wave end, trigger auto-spawn
    void Update(float dt, GameData& data, WorldSystem& worldSystem, EnemyFactory& enemyFactory);

    // Start the next wave; no-op if one is already active
    void StartWave(GameData& data, const EnemyFactory& enemyFactory);

    // (Re)prime the 1-wave lookahead so the next wave to launch is currentWaveNumber + 1.
    // Called with 0 on a fresh load and with the restored wave number when continuing a save.
    void PrepareForWave(int currentWaveNumber, const EnemyFactory& enemyFactory);

    void ToggleAutoSpawn() { m_autoSpawn = !m_autoSpawn; }
    bool IsAutoSpawn() const { return m_autoSpawn; }

    // --- Read access for the HUD (composition of the upcoming wave, win condition, scaling) ---
    const WaveDef& GetNextWaveDef() const { return m_pendingDef; }
    float GetNextWaveBudget() const { return m_pendingBudget; } // threat budget of the upcoming wave
    int GetVictoryWave() const { return m_victoryWave; } // 0 = endless
    int GetUpgradeTier() const { return m_activeTier; }

    // Fully-upgraded prototype enemies for the upcoming (pending) wave, keyed by enemy type. The HUD
    // reads stats/level straight from these; the spawn path clones from the promoted copy of this set.
    const std::unordered_map<std::string, Enemy>& GetPreviewPrototypes() const { return m_previewPrototypes; }

private:
    // One enemy type the procedural generator may draw from, with its threat cost and pacing.
    struct PoolEntry {
        std::string m_enemy;
        int m_cost = 1;
        int m_minWave = 1;        // earliest wave this type may appear in
        float m_interval = 1.0f;  // seconds between spawns of this type within a wave
    };

    // A single resolved spawn event in the active wave's queue
    struct PendingSpawn {
        std::string m_type;
        int m_nest;
        float m_time; // seconds after wave start when this enemy should spawn
    };

    // Procedurally compose a wave from the threat budget for that wave number.
    WaveDef GenerateWave(int waveNumber);

    // Total threat budget allotted to a wave before any spending (the value the HUD displays).
    float BudgetForWave(int waveNumber) const;

    // Look up a pool entry by enemy name; nullptr if absent (e.g. an unlisted boss).
    const PoolEntry* FindPoolEntry(const std::string& name) const;

    // Upgrade tier applied to enemies spawned in the given wave (0 = none).
    int UpgradeTierFor(int waveNumber) const;

    // Apply the current upgrade tier to a freshly created enemy before it spawns.
    void ApplyTierUpgrades(Enemy& enemy, int tier, const EnemyFactory& enemyFactory) const;

    // Rebuild the preview prototype pool for m_pendingDef, upgraded to the pending wave's tier.
    void RebuildPreviewPrototypes(int pendingWaveNumber, const EnemyFactory& enemyFactory);

    // Expands a WaveDef into a sorted PendingSpawn list.
    void BuildSpawnQueue(const WaveDef& def, int nestCount);

    // --- Configuration parsed from waves.toml ---
    float m_baseBudget = 20.0f;
    BudgetType m_budgetType = BudgetType::Exponential; // scaling model; default when "type" is absent
    float m_growthRate = 0.15f;     // Exponential: per-wave growth fraction
    float m_linearStep = 10.0f;     // Step-Linear: budget added per wave
    float m_tierAdjustment = 0.0f;  // Step-Linear: budget removed per upgrade tier
    int m_victoryWave = 0;          // 0 = endless, >0 = victory after this wave is cleared
    int m_bossInterval = 0;         // boss wave every Nth wave; 0 disables
    std::vector<std::string> m_bossEnemies;
    int m_upgradeInterval = 0;      // upgrade tier increments every Nth wave; 0 disables
    std::vector<PoolEntry> m_enemyPool;

    // --- 1-wave lookahead: both the next wave and the one after are pre-generated ---
    WaveDef m_pendingDef;   // composition for the next wave to launch (waveNumber + 1)
    WaveDef m_lookaheadDef; // composition for the wave after that (promoted on StartWave)
    float m_pendingBudget = 0.0f;   // total budget of m_pendingDef's wave
    float m_lookaheadBudget = 0.0f; // total budget of m_lookaheadDef's wave
    int m_activeTier = 0;   // upgrade tier of the wave currently spawning

    // Pre-upgraded prototype enemies, cloned on spawn and read by the HUD.
    // m_previewPrototypes corresponds to m_pendingDef (the upcoming wave, shown by the HUD);
    // m_spawnPrototypes is the wave currently spawning, moved from preview on StartWave.
    std::unordered_map<std::string, Enemy> m_previewPrototypes;
    std::unordered_map<std::string, Enemy> m_spawnPrototypes;

    std::mt19937 m_rng;

    std::vector<PendingSpawn> m_pendingSpawns;
    int m_nextSpawn = 0;     // index of the next unprocessed pending spawn
    float m_elapsed = 0.0f;  // seconds elapsed since the current wave started

    bool m_autoSpawn = false;
    float m_autoSpawnTimer = 0.0f;
};
