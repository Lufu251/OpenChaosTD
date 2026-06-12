#include <systems/world_system.hpp>
#include <world/game_data.hpp>
#include <world/tile.hpp>
#include <factory/enemy_factory.hpp>
#include <engine/systems/sound_system.hpp>
#include <raymath.h>
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
    enemy.m_position = {
        static_cast<float>(gameData.m_map.GetNests()[nest].first * gameData.m_map.GetTileSize() + static_cast<float>(gameData.m_map.GetTileSize()) /2),
        static_cast<float>(gameData.m_map.GetNests()[nest].second * gameData.m_map.GetTileSize()+ static_cast<float>(gameData.m_map.GetTileSize()) /2)
    };

    enemy.m_spawnedNest = nest;
    enemy.m_waypointIndex = gameData.m_map.GetPaths()[nest].size() -2;

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
        gameData.m_lives -= gameData.m_enemies.Get(erase)->GetBaseStats()->m_livesOnReach;
        RemoveEnemy(erase, gameData);
    }
}

void WorldSystem::CheckEnemyDead(GameData& gameData, EnemyFactory& enemyFactory, ParticleSystem& particles, SoundSystem& sound){
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

        // Unit vector pointing back along the path (away from the core), used to fan the children out.
        // Children are only ever pushed backward, so they never skip ahead or reach the core early.
        Vector2 back = {0.0f, 0.0f};
        const auto& path = gameData.m_map.GetPaths()[nest];
        if (waypoint >= 0 && waypoint < static_cast<int>(path.size())) {
            Vector2 toWp = Vector2Subtract(path[waypoint], pos);
            float dist = Vector2Length(toWp);
            if (dist > 0.001f) // guard against NaN when the parent sits exactly on the waypoint
                back = Vector2Scale(toWp, -1.0f / dist);
        }
        float tileSize = static_cast<float>(gameData.m_map.GetTileSize());

        // Spawn children after parent is removed, staggered backward along the path by req.m_spacing
        // so they spread out instead of stacking on the exact death position.
        for (const auto& req : requests) {
            for (int i = 0; i < req.m_count; i++) {
                float offset = i * req.m_spacing;
                auto built = enemyFactory.Create(req.m_type);
                if (!built) break; // unknown split type (edited datapack) -> spawn nothing
                Enemy child = std::move(*built);
                child.m_position     = Vector2Add(pos, Vector2Scale(back, offset));
                child.m_spawnedNest  = nest;
                child.m_waypointIndex = waypoint;
                // Progress rises away from the core, matching the backward push, so targeting (First/
                // Last) sees distinct values instead of ties.
                child.m_progress     = progress + (tileSize > 0.0f ? offset / tileSize : 0.0f);
                gameData.m_enemies.Insert(std::move(child));
            }
        }
    }
}


void WorldSystem::CheckGameOver(bool& gameOver, GameData& gameData){
    // Core live reaches zero
    if(gameData.m_lives <= 0){
        gameOver = true;
    }
}
