#pragma once

#include <vector>
#include <engine/lib/dense_slotmap.hpp>
#include <engine/systems/particle_system.hpp>
#include <world/tower.hpp>
#include <world/enemy.hpp>
#include <world/attack.hpp>

class SoundSystem;

class TowerSystem{
public:
    void Update(float dt, DenseSlotMap<Tower>& towers, DenseSlotMap<Enemy>& enemies, std::vector<Attack>& attacks, ParticleSystem& particles, SoundSystem& sound);
    void TickAttacks(float dt, DenseSlotMap<Enemy>& enemies, std::vector<Attack>& attacks, ParticleSystem& particles);
    std::vector<DenseSlotMap<Enemy>::Key> FindTargets(const Tower& tower, DenseSlotMap<Enemy>& enemies, int maxTargets);

private:
    void RecomputeStats(Tower& tower, float dt);
    void DecayAttackFlash(Tower& tower, float dt);
    void Fire(Tower& tower, const std::vector<DenseSlotMap<Enemy>::Key>& targetKeys, DenseSlotMap<Enemy>& enemies, std::vector<Attack>& attacks, ParticleSystem& particles, SoundSystem& sound);
    static AttackVisual BuildVisual(const Tower& tower, std::vector<Vector2> targetPositions);
    void BuildPayload(const Tower& tower, AttackPayload& payload);
    bool CompareTarget(const Enemy& a, const Enemy& b, TargetingMode mode);
    std::vector<Enemy*> FindEnemiesInRange(const Tower& tower, DenseSlotMap<Enemy>& enemies);
};
