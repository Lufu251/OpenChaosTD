#include <systems/wave_manager.hpp>
#include <app/game_data.hpp>
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <iostream>

void WaveManager::Load(FileStore& fileStore, const EnemyFactory& enemyFactory, const std::string& dataDir) {
    m_rng.seed(std::random_device{}());

    std::string path = dataDir + "/waves.toml";
    if (!fileStore.Exists(path)) {
        std::cerr << "WaveManager: " << path << " not found\n";
        return;
    }

    auto data = fileStore.LoadToml(path);

    // Budget scaling and win condition
    if (auto b = data["budget"].as_table()) {
        m_baseBudget = (*b)["base_budget"].value_or(m_baseBudget);

        // Scaling model: "step_linear" or "exponential" (the default for an absent/unknown type).
        std::string type = (*b)["type"].value_or(std::string{});
        m_budgetType = (type == "step_linear") ? BudgetType::StepLinear : BudgetType::Exponential;
        m_growthRate = (*b)["growth_rate"].value_or(m_growthRate);
        m_linearStep = (*b)["linear_step"].value_or(m_linearStep);
        m_tierAdjustment = (*b)["tier_adjustment"].value_or(m_tierAdjustment);

        m_victoryWave = (*b)["victory_wave"].value_or(m_victoryWave);
    }

    // Periodic boss waves
    if (auto boss = data["boss"].as_table()) {
        m_bossInterval = (*boss)["interval"].value_or(m_bossInterval);
        if (auto names = (*boss)["boss_enemies"].as_array())
            for (auto&& name : *names)
                if (auto s = name.value<std::string>()) m_bossEnemies.push_back(*s);
    }

    m_upgradeInterval = data["upgrade_interval"].value_or(m_upgradeInterval);

    // Enemy pool the generator draws from
    if (auto pool = data["enemy_pool"].as_array()) {
        for (auto&& eNode : *pool) {
            auto e = eNode.as_table();
            if (!e) continue;
            PoolEntry entry;
            entry.m_enemy = (*e)["enemy"].value_or(std::string{});
            entry.m_cost = (*e)["cost"].value_or(1);
            entry.m_minWave = (*e)["min_wave"].value_or(1);
            entry.m_interval = (*e)["interval"].value_or(1.0f);
            m_enemyPool.push_back(std::move(entry));
        }
    }

    // Prime the 1-wave lookahead for a fresh game (next wave to launch is wave 1).
    PrepareForWave(0, enemyFactory);

    std::cout << "WaveManager: loaded " << m_enemyPool.size() << " pool entries, victory_wave="
              << m_victoryWave << "\n";
}

void WaveManager::PrepareForWave(int currentWaveNumber, const EnemyFactory& enemyFactory) {
    int next = currentWaveNumber + 1;

    // 1-wave lookahead: pre-generate the next wave and the one after it before any start.
    m_pendingDef = GenerateWave(next);
    m_lookaheadDef = GenerateWave(next + 1);
    m_pendingBudget = BudgetForWave(next);
    m_lookaheadBudget = BudgetForWave(next + 1);

    // Build the prototype pool previewing the next wave to launch.
    RebuildPreviewPrototypes(next, enemyFactory);

    // No wave is in progress yet.
    m_pendingSpawns.clear();
    m_nextSpawn = 0;
    m_elapsed = 0.0f;
    m_activeTier = 0;
}

const WaveManager::PoolEntry* WaveManager::FindPoolEntry(const std::string& name) const {
    for (const auto& e : m_enemyPool)
        if (e.m_enemy == name) return &e;
    return nullptr;
}

int WaveManager::UpgradeTierFor(int waveNumber) const {
    if (m_upgradeInterval <= 0) return 0;
    return waveNumber / m_upgradeInterval;
}

float WaveManager::BudgetForWave(int waveNumber) const {
    if (m_budgetType == BudgetType::StepLinear) {
        // Linear growth with a dip each upgrade tier (tougher enemies, fewer of them). Clamped so the
        // tier subtraction can never push a wave below the base budget.
        int tier = UpgradeTierFor(waveNumber);
        float budget = m_baseBudget + m_linearStep * waveNumber - m_tierAdjustment * tier;
        return std::max(budget, m_baseBudget);
    }
    // Exponential (default): smooth compounding growth.
    return m_baseBudget * std::pow(1.0f + m_growthRate, static_cast<float>(waveNumber - 1));
}

WaveManager::WaveDef WaveManager::GenerateWave(int waveNumber) {
    WaveDef def;
    if (m_enemyPool.empty()) return def;

    float budget = BudgetForWave(waveNumber);

    // Selection pool: types unlocked by this wave and not reserved as bosses.
    std::vector<const PoolEntry*> pool;
    for (const auto& e : m_enemyPool) {
        if (e.m_minWave > waveNumber) continue;
        if (std::find(m_bossEnemies.begin(), m_bossEnemies.end(), e.m_enemy) != m_bossEnemies.end())
            continue;
        pool.push_back(&e);
    }

    // Boss wave: force exactly one boss and bill its cost against the budget; the rest funds escorts.
    bool isBossWave = m_bossInterval > 0 && (waveNumber % m_bossInterval) == 0 && !m_bossEnemies.empty();
    if (isBossWave) {
        std::uniform_int_distribution<std::size_t> pick(0, m_bossEnemies.size() - 1);
        const std::string& bossName = m_bossEnemies[pick(m_rng)];
        const PoolEntry* boss = FindPoolEntry(bossName);

        SpawnGroup g;
        g.m_enemyType = bossName;
        g.m_count = 1;
        g.m_nest = -1;
        g.m_interval = boss ? boss->m_interval : 1.5f;
        def.m_groups.push_back(std::move(g));

        if (boss) budget -= static_cast<float>(boss->m_cost);
        if (budget < 0.0f) budget = 0.0f;
    }

    if (pool.empty()) return def; // boss-only wave (or nothing unlocked yet)

    // Cheapest unit sets the loop's termination floor.
    int cheapest = pool.front()->m_cost;
    for (const auto* e : pool) cheapest = std::min(cheapest, e->m_cost);

    // Semi-random selection: draw affordable units until the budget can't afford the cheapest.
    std::unordered_map<std::string, int> tally;
    while (budget >= static_cast<float>(cheapest)) {
        std::vector<const PoolEntry*> affordable;
        for (const auto* e : pool)
            if (static_cast<float>(e->m_cost) <= budget) affordable.push_back(e);
        if (affordable.empty()) break;

        std::uniform_int_distribution<std::size_t> pick(0, affordable.size() - 1);
        const PoolEntry* chosen = affordable[pick(m_rng)];
        tally[chosen->m_enemy]++;
        budget -= static_cast<float>(chosen->m_cost);
    }

    // Group identical selections into one spawn group each.
    for (const auto& [name, count] : tally) {
        const PoolEntry* e = FindPoolEntry(name);
        SpawnGroup g;
        g.m_enemyType = name;
        g.m_count = count;
        g.m_nest = -1;
        g.m_interval = e ? e->m_interval : 1.0f;
        g.m_delay = 0.0f;
        def.m_groups.push_back(std::move(g));
    }

    return def;
}

void WaveManager::ApplyTierUpgrades(Enemy& enemy, int tier, const EnemyFactory& enemyFactory) const {
    // Single source of truth lives in EnemyFactory so the world layer can scale spawned children too.
    enemyFactory.ApplyTierUpgrades(enemy, tier);
}

void WaveManager::RebuildPreviewPrototypes(int pendingWaveNumber, const EnemyFactory& enemyFactory) {
    m_previewPrototypes.clear();
    int tier = UpgradeTierFor(pendingWaveNumber);

    // One fully-upgraded prototype per unique enemy type in the upcoming wave.
    for (const auto& grp : m_pendingDef.m_groups) {
        if (m_previewPrototypes.count(grp.m_enemyType)) continue;
        auto proto = enemyFactory.Create(grp.m_enemyType); // e.g. an unlisted type — nothing to clone
        if (!proto) continue;
        ApplyTierUpgrades(*proto, tier, enemyFactory);
        proto->RecomputeLive(); // refresh live speed/armor so the HUD shows upgraded values
        m_previewPrototypes.emplace(grp.m_enemyType, std::move(*proto));
    }
}

void WaveManager::BuildSpawnQueue(const WaveDef& def, int nestCount) {
    m_pendingSpawns.clear();
    m_nextSpawn = 0;
    m_elapsed = 0.0f;

    if (nestCount <= 0) return; // no nests — nothing to spawn from, and avoids modulo-by-zero

    int nestIdx = 0;
    for (const auto& grp : def.m_groups) {
        for (int i = 0; i < grp.m_count; i++) {
            PendingSpawn ps;
            ps.m_type = grp.m_enemyType;
            ps.m_nest = (grp.m_nest >= 0) ? grp.m_nest : (nestIdx++ % nestCount);
            ps.m_time = grp.m_delay + i * grp.m_interval;
            m_pendingSpawns.push_back(ps);
        }
    }

    std::sort(m_pendingSpawns.begin(), m_pendingSpawns.end(),
        [](const PendingSpawn& a, const PendingSpawn& b) { return a.m_time < b.m_time; });
}

void WaveManager::Update(float dt, GameData& data, WorldSystem& worldSystem, EnemyFactory& enemyFactory) {
    if (data.m_waveActive) {
        m_elapsed += dt;

        // Fire any spawns whose scheduled time has arrived, cloning from the pre-upgraded prototypes
        while (m_nextSpawn < static_cast<int>(m_pendingSpawns.size())
               && m_pendingSpawns[m_nextSpawn].m_time <= m_elapsed) {
            const auto& ps = m_pendingSpawns[m_nextSpawn++];
            auto it = m_spawnPrototypes.find(ps.m_type);
            if (it == m_spawnPrototypes.end()) continue; // unknown type — no prototype to clone
            worldSystem.SpawnEnemy(ps.m_nest, it->second.Clone(), data);
        }

        // Wave ends once the spawn queue is exhausted and all enemies are cleared
        bool queueDone = m_nextSpawn >= static_cast<int>(m_pendingSpawns.size());
        if (queueDone && data.m_enemies.Size() == 0) {
            data.m_waveActive = false;
            m_autoSpawnTimer = 0.0f;

            // Win condition is driven entirely by waves.toml (victory_wave); 0 = endless.
            if (m_victoryWave > 0 && data.m_waveNumber >= m_victoryWave)
                data.m_victory = true;
        }
        return;
    }

    // Auto-spawn: start the next wave after the configured delay
    if (m_autoSpawn && data.m_waveNumber > 0 && !data.m_victory) {
        m_autoSpawnTimer += dt;
        if (m_autoSpawnTimer >= data.m_autoSpawnDelay)
            StartWave(data, enemyFactory);
    }
}

void WaveManager::StartWave(GameData& data, const EnemyFactory& enemyFactory) {
    if (data.m_waveActive) return;

    // Refuse to start a wave that would spawn nothing — an empty/misconfigured enemy pool or a map
    // with no nests would otherwise build an empty queue that auto-completes on the first frame (and
    // could trip the victory condition with no combat). Reset the auto-spawn timer so this retries at
    // most once per delay rather than every frame.
    int nestCount = static_cast<int>(data.m_map.GetNests().size());
    int pendingCount = 0;
    for (const auto& grp : m_pendingDef.m_groups)
        pendingCount += grp.m_count;
    if (nestCount <= 0 || pendingCount <= 0) {
        std::cerr << "WaveManager: skipping empty wave " << (data.m_waveNumber + 1)
                  << " (no spawns generated)\n";
        m_autoSpawnTimer = 0.0f;
        return;
    }

    data.m_waveNumber++;
    data.m_waveActive = true;

    m_activeTier = UpgradeTierFor(data.m_waveNumber);

    BuildSpawnQueue(m_pendingDef, nestCount); // m_pendingDef holds this wave's composition

    // The preview prototypes were already upgraded to exactly this wave's tier, so promote them to
    // the active spawn set — Update clones from these without re-running the upgrade path.
    m_spawnPrototypes = std::move(m_previewPrototypes);

    // Promote the lookahead to pending and pre-generate the new lookahead (keeps one wave ahead).
    m_pendingDef = std::move(m_lookaheadDef);
    m_lookaheadDef = GenerateWave(data.m_waveNumber + 2);
    m_pendingBudget = m_lookaheadBudget;
    m_lookaheadBudget = BudgetForWave(data.m_waveNumber + 2);

    // Rebuild the preview pool for the new upcoming wave (next wave's tier) for the HUD.
    RebuildPreviewPrototypes(data.m_waveNumber + 1, enemyFactory);
}
