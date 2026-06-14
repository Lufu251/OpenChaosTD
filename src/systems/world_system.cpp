#include <systems/world_system.hpp>
#include <app/game_data.hpp>
#include <world/tile.hpp>
#include <content/enemy_factory.hpp>
#include <engine/systems/sound_system.hpp>
#include <raymath.h>
#include <iostream>
#include <vector>

bool WorldSystem::PlaceTower(int x, int y, Tower& tower, GameData& gameData){
    if(!ValidateTowerPlacement(x, y, gameData)) return false;

    Tile& tile = gameData.m_map.Get(x, y);
    tower.m_position = Vector2Add(gameData.m_map.TileToWorld(x, y), {gameData.m_map.GetTileSize() /2.f, gameData.m_map.GetTileSize() /2.f});

    // Bake any terrain buff into the tower's base stats before it moves into the slotmap. The
    // modifier stays on the tile, so a tower placed here later is buffed again; selling/destroying
    // a tower erases it entirely, which reverts the buff with no extra bookkeeping.
    if (tile.m_modifier.Active())
        tower.PatchStats(tile.m_modifier.m_statKey, tile.m_modifier.m_value, tile.m_modifier.m_mul);

    DenseSlotMap<Tower>::Key towerKey = gameData.m_towers.Insert(std::move(tower));

    tile.m_walkable = false;
    tile.m_buildable = false;
    tile.m_towerKey = towerKey;
    return true;
}

void WorldSystem::RemoveTower(int x, int y, GameData& gameData){
    Tile& tile = gameData.m_map.Get(x, y);

    if(tile.m_towerKey != DenseSlotMap<Tower>::INVALID_KEY){
        // Remove tower
        gameData.m_towers.Erase(tile.m_towerKey);

        tile.m_walkable = true;
        tile.m_buildable = true;
        tile.m_towerKey = DenseSlotMap<Tower>::INVALID_KEY;

        gameData.m_map.BuildPathMesh();
    }
}

bool WorldSystem::ValidateTowerPlacement(int x, int y, GameData& gameData){
    Tile& tile = gameData.m_map.Get(x, y);

    // Return if tile not buildable
    if(!tile.m_buildable)
        return false;

    // Check if paths are still valid after tower placement
    tile.m_walkable = false;
    gameData.m_map.BuildPathMesh();
    if(!gameData.m_map.ValidatePathMesh()){
        tile.m_walkable = true;
        gameData.m_map.BuildPathMesh();
        return false;
    }

    // If nothing fails allow tower placement
    return true;
}

void WorldSystem::SpawnEnemy(int nest, Enemy&& enemy, GameData& gameData){
    // Guard the nest index and its path before indexing: an out-of-range nest is undefined
    // behaviour, and a path shorter than two nodes would make the size()-2 waypoint index
    // (computed in unsigned arithmetic) underflow into a garbage value. Skip the spawn instead.
    const auto& nests = gameData.m_map.GetNests();
    const auto& paths = gameData.m_map.GetPaths();
    if (nest < 0 || nest >= static_cast<int>(nests.size()) || nest >= static_cast<int>(paths.size())) {
        std::cerr << "SpawnEnemy: nest index " << nest << " out of range; skipping spawn\n";
        return;
    }
    const std::vector<Vector2>& path = paths[nest];
    if (path.size() < 2) {
        std::cerr << "SpawnEnemy: nest " << nest << " has no usable path; skipping spawn\n";
        return;
    }

    const int tileSize = gameData.m_map.GetTileSize();
    enemy.m_position = {
        static_cast<float>(nests[nest].first * tileSize) + static_cast<float>(tileSize) / 2,
        static_cast<float>(nests[nest].second * tileSize) + static_cast<float>(tileSize) / 2
    };

    enemy.m_spawnedNest = nest;
    enemy.m_waypointIndex = static_cast<int>(path.size()) - 2;

    gameData.m_enemies.Insert(std::move(enemy));
}

void WorldSystem::RemoveEnemy(DenseSlotMap<Enemy>::Key key, GameData& gameData){
    gameData.m_enemies.Erase(key);
}

void WorldSystem::CheckEnemyReachedCore(GameData& gameData){
    std::vector<DenseSlotMap<Enemy>::Key> enemyErase;
    for (auto& enemy: gameData.m_enemies) {
        // Enemy reached core
        if(enemy.m_waypointIndex == -1){
            enemyErase.push_back(gameData.m_enemies.KeyOf(&enemy));
        }
    }

    for(auto& erase : enemyErase){
        Enemy* enemy = gameData.m_enemies.Get(erase);
        if (!enemy) continue; // defensive: keys are distinct so this shouldn't happen, but don't deref null
        gameData.m_lives -= enemy->GetBaseStats()->m_livesOnReach;
        RemoveEnemy(erase, gameData);
    }
}

void WorldSystem::CheckEnemyDead(GameData& gameData, EnemyFactory& enemyFactory, ParticleSystem& particles, SoundSystem& sound, int tier){
    std::vector<DenseSlotMap<Enemy>::Key> toRemove;
    for (auto& enemy : gameData.m_enemies) {
        if (enemy.m_currentHealth <= 0.0f)
            toRemove.push_back(gameData.m_enemies.KeyOf(&enemy));
    }
    for (auto& key : toRemove) {
        Enemy* enemy = gameData.m_enemies.Get(key);
        gameData.m_gold += enemy->GetBaseStats()->m_reward;

        // Copy parent path state and collect spawn requests before mutating the slotmap
        Vector2 pos          = enemy->m_position;
        int     nest         = enemy->m_spawnedNest;
        int     waypoint     = enemy->m_waypointIndex;
        float   progress     = enemy->m_progress;

        std::vector<SpawnRequest> requests;
        for (const auto& mod : enemy->m_modules) {
            auto req = mod->OnDeath();
            if (req && enemyFactory.Has(req->m_type))
                requests.push_back(*req);
        }

        // Death burst — pointer into EmitterPresets, set at enemy creation time
        if (enemy->m_presentation.m_deathDescPtr)
            particles.Emit(pos, *enemy->m_presentation.m_deathDescPtr);

        sound.PlaySfx(enemy->m_presentation.m_deathSound); // defaults to "enemy_death"

        RemoveEnemy(key, gameData);

        // Spawn children after the parent is removed, scaled to the active wave tier.
        for (const auto& req : requests)
            SpawnFromRequest(req, pos, nest, waypoint, progress, gameData, enemyFactory, tier);
    }
}

void WorldSystem::SpawnChildren(const std::vector<PendingChildSpawn>& spawns, GameData& gameData, EnemyFactory& enemyFactory, int tier){
    // Periodic-summon children collected during the enemy tick; placed here (after the tick) so the
    // enemy slotmap is never mutated mid-iteration.
    for (const auto& s : spawns)
        SpawnFromRequest(s.m_request, s.m_position, s.m_nest, s.m_waypointIndex, s.m_progress, gameData, enemyFactory, tier);
}

void WorldSystem::SpawnFromRequest(const SpawnRequest& req, Vector2 pos, int nest, int waypoint, float progress,
                                   GameData& gameData, EnemyFactory& enemyFactory, int tier){
    if (!enemyFactory.Has(req.m_type)) return; // unknown type (edited datapack) -> spawn nothing

    // Guard the nest index before indexing, mirroring SpawnEnemy: a stale nest (e.g. after a map
    // change) would otherwise be an out-of-bounds vector access.
    const auto& paths = gameData.m_map.GetPaths();
    if (nest < 0 || nest >= static_cast<int>(paths.size())) return;

    // Unit vector pointing back along the path (away from the core), used to fan the children out.
    // Children are only ever pushed backward, so they never skip ahead or reach the core early.
    Vector2 back = {0.0f, 0.0f};
    const auto& path = paths[nest];
    if (waypoint >= 0 && waypoint < static_cast<int>(path.size())) {
        Vector2 toWp = Vector2Subtract(path[waypoint], pos);
        float dist = Vector2Length(toWp);
        if (dist > 0.001f) // guard against NaN when the parent sits exactly on the waypoint
            back = Vector2Scale(toWp, -1.0f / dist);
    }
    float tileSize = static_cast<float>(gameData.m_map.GetTileSize());

    // Staggered backward along the path by req.m_spacing so children spread out instead of stacking.
    for (int i = 0; i < req.m_count; i++) {
        float offset = i * req.m_spacing;
        auto built = enemyFactory.Create(req.m_type);
        if (!built) break;
        Enemy child = std::move(*built);
        // Scale the child to the spawning wave's tier so late-wave splits/summons stay threatening,
        // matching the tier applied to enemies spawned directly by the wave manager.
        enemyFactory.ApplyTierUpgrades(child, tier);
        child.RecomputeLive(); // refresh live speed/armor after the tier patches
        child.m_position     = Vector2Add(pos, Vector2Scale(back, offset));
        child.m_spawnedNest  = nest;
        child.m_waypointIndex = waypoint;
        // Progress rises away from the core, matching the backward push, so targeting (First/Last)
        // sees distinct values instead of ties.
        child.m_progress     = progress + (tileSize > 0.0f ? offset / tileSize : 0.0f);
        gameData.m_enemies.Insert(std::move(child));
    }
}


void WorldSystem::CheckGameOver(bool& gameOver, GameData& gameData){
    // Core live reaches zero
    if(gameData.m_lives <= 0){
        gameOver = true;
    }
}
