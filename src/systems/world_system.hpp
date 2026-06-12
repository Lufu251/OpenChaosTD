#pragma once

#include <engine/lib/dense_slotmap.hpp>
#include <engine/systems/particle_system.hpp>

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
    void CheckEnemyDead(GameData& gameData, EnemyFactory& enemyFactory, ParticleSystem& particles, SoundSystem& sound);
    void CheckGameOver(bool& gameOver, GameData& gameData);

private:
    bool ValidateTowerPlacement(int x, int y, GameData& gameData);
};