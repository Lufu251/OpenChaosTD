#include <systems/enemy_system.hpp>

#include <raymath.h>
#include <algorithm>

void EnemySystem::FollowPath(float dt, DenseSlotMap<Enemy>& enemies, const Map& map){
    for (auto& enemy : enemies) {
        float speed = enemy.GetBaseStats()->m_liveSpeed;
        if (speed <= 0.0f) continue;

        float remainingTime = dt;

        while (remainingTime > 0.0f && enemy.m_waypointIndex >= 0) {
            Vector2 toTarget = Vector2Subtract(map.GetPaths()[enemy.m_spawnedNest][enemy.m_waypointIndex], enemy.m_position);
            float distToTarget = Vector2Length(toTarget);
            float moveDistance = speed * remainingTime;

            if (moveDistance >= distToTarget) {
                remainingTime -= distToTarget / speed;
                enemy.m_position = map.GetPaths()[enemy.m_spawnedNest][enemy.m_waypointIndex];
                enemy.m_waypointIndex--;
                enemy.m_progress = static_cast<float>(enemy.m_waypointIndex + 1);
            } else {
                Vector2 direction = Vector2Normalize(toTarget);
                enemy.m_position = Vector2Add(enemy.m_position, Vector2Scale(direction, moveDistance));
                remainingTime = 0.0f;
                float remainingDist = distToTarget - moveDistance;
                enemy.m_progress = enemy.m_waypointIndex + remainingDist / map.GetTileSize();
            }
        }
    }
}

void EnemySystem::TickEnemies(float dt, DenseSlotMap<Enemy>& enemies, const Map& map, ParticleSystem& particles,
                             std::vector<PendingChildSpawn>& outSpawns){
    for (auto& enemy : enemies) {
        // Recompute live combat stats (speed, armor) from base + module contributions.
        // Mirrors TowerSystem::RecomputeStats.
        enemy.RecomputeLive();
        BaseStatsModule* base = enemy.GetBaseStats();

        // Modules may request minion spawns this tick (SummonerModule). Collect them per enemy, then
        // attach the parent's path state so the world layer can place them after the iteration.
        std::vector<SpawnRequest> reqs;
        for (auto& mod : enemy.m_modules)
            mod->Tick(dt, enemy, reqs);
        for (auto& req : reqs)
            outSpawns.push_back({req, enemy.m_position, enemy.m_spawnedNest, enemy.m_waypointIndex, enemy.m_progress});

        // Compute enemy velocity so status-effect particles inherit it and don't lag behind
        Vector2 baseVel = {0, 0};
        if (enemy.m_waypointIndex >= 0) {
            Vector2 toWaypoint = Vector2Subtract(
                map.GetPaths()[enemy.m_spawnedNest][enemy.m_waypointIndex],
                enemy.m_position
            );
            float dist = Vector2Length(toWaypoint);
            if (dist > 0.001f)
                baseVel = Vector2Scale(Vector2Normalize(toWaypoint), base->m_liveSpeed);
        }

        for (auto& effect : enemy.m_effects) {
            // Drive the engine-owned continuous emitter for this effect (burn flicker, slow drift).
            // Register once, then refresh its anchor each frame so the emitter follows the enemy.
            if (effect.m_particleDesc.m_emitRate > 0.0f) {
                if (effect.m_emitter == DenseSlotMap<Emitter>::INVALID_KEY)
                    effect.m_emitter = particles.AddEmitter(effect.m_particleDesc, enemy.m_position, baseVel);
                else
                    particles.UpdateEmitter(effect.m_emitter, enemy.m_position, baseVel);
            }

            // Status effects mutate the core module's live mirror directly (reset from base each
            // tick), so they stay outside the persistent PatchStats/EnemyUpgrade pipeline by design.
            switch (effect.m_type) {
                case EffectType::Burn:
                    enemy.m_currentHealth -= effect.m_value * dt;
                    break;
                case EffectType::Slow:
                    // m_value is slow strength as a percent (90 = 90% slower)
                    base->m_liveSpeed *= 1.0f - effect.m_value / 100.0f;
                    break;
                case EffectType::ArmorShred:
                    // Flat armor reduction; floored at 0
                    base->m_liveArmor = std::max(0.0f, base->m_liveArmor - effect.m_value);
                    break;
                case EffectType::Stun:
                    base->m_liveSpeed = 0.0f; // FollowPath skips enemies with speed <= 0
                    break;
                case EffectType::Weakness:
                    break; // consumed on hit in TowerSystem::TickAttacks
            }
            effect.m_duration -= dt;
        }
        // Drop expired effects, releasing their live emitter promptly (no-op on an invalid handle).
        std::erase_if(enemy.m_effects, [&particles](const Effect& e){
            if (e.m_duration > 0.0f) return false;
            particles.RemoveEmitter(e.m_emitter);
            return true;
        });
    }
}
