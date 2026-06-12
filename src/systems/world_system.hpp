#pragma once

#include <vector>
#include <engine/lib/dense_slotmap.hpp>
#include <engine/systems/particle_system.hpp>
#include <world/modules.hpp> // SpawnRequest, PendingChildSpawn

struct GameData;
class Tower;
class Enemy;
class EnemyFactory;
class SoundSystem;

class WorldSystem{
public:
    bool PlaceTower(int x, int y, Tower& tower, GameData& gameData);
    void RemoveTower(int x, int y, GameData& gameData);
    void SpawnEnemy(int nest, Enemy&& enemy, GameData& gameData);
    void RemoveEnemy(DenseSlotMap<Enemy>::Key key, GameData& gameData);

    void CheckEnemyReachedCore(GameData& gameData);
    // tier is the active wave's upgrade tier; on-death children (Split) are scaled to it.
    void CheckEnemyDead(GameData& gameData, EnemyFactory& enemyFactory, ParticleSystem& particles, SoundSystem& sound, int tier);
    void CheckGameOver(bool& gameOver, GameData& gameData);

    // Place the children collected during the enemy tick (e.g. SummonerModule), scaled to the wave tier.
    void SpawnChildren(const std::vector<PendingChildSpawn>& spawns, GameData& gameData, EnemyFactory& enemyFactory, int tier);

private:
    bool ValidateTowerPlacement(int x, int y, GameData& gameData);
    // Build req.m_count children of req.m_type, fanned backward along the path from the parent's path
    // state, each scaled to the wave tier. Shared by the death-split and periodic-summon paths.
    void SpawnFromRequest(const SpawnRequest& req, Vector2 pos, int nest, int waypoint, float progress,
                          GameData& gameData, EnemyFactory& enemyFactory, int tier);
};