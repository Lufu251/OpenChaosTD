#pragma once

#include <raylib.h>
#include <engine/lib/object_pool.hpp>
#include <engine/lib/dense_slotmap.hpp>

// Spawn shape — offsets each particle from the emitter's center anchor at spawn time.
enum class SpawnShape { Point, Line, Box, Circle, Ring };

struct EmitterDesc {
    Color m_color = WHITE;
    Color m_endColor = {255, 255, 255, 0};
    int m_count = 0;         // particles per burst (Emit) or per emission step (live emitter); 0 = disabled
    float m_speed = 50.0f;
    float m_speedVariance = 20.0f;
    float m_spread = 360.0f; // degrees; total arc of the spawn cone (360 = full circle)
    float m_angle = 0.0f;    // degrees; 0=right, 90=down, 180=left, 270=up
    float m_lifetime = 0.2f;
    float m_size = 3.0f;
    float m_endSize = 0.0f;

    // Spawn shape — particles are offset from the center anchor; radial/tangential math still
    // references that anchor so shaped spawns expand/spin correctly.
    SpawnShape m_shape = SpawnShape::Point;
    Vector2 m_shapeSize = {0, 0}; // Box: full width/height; Line: x = length (oriented by m_angle)
    float m_shapeRadius = 0.0f;   // Circle: max radius; Ring: fixed radius on the circumference

    // Unified dynamics — added on top of the linear cone velocity, recomputed each frame.
    float m_radialSpeed = 0.0f;     // + = outward from center, - = inward
    float m_tangentialSpeed = 0.0f; // perpendicular to radial → spin around the center

    // Continuous emission — particles/sec for a live emitter; 0 = burst-only.
    float m_emitRate = 0.0f;
};

// Internal per-particle runtime state.
struct Particle {
    Vector2 position = {};
    Vector2 velocity = {};     // linear component (cone direction + inherited base velocity)
    Vector2 center = {};       // anchor captured at spawn; radial/tangential are relative to this
    Vector2 seedDir = {1, 0};  // fallback radial direction while the particle sits on the center
    float radialSpeed = 0.0f;
    float tangentialSpeed = 0.0f;
    float lifetime = 0.0f;
    float maxLifetime = 0.0f;
    Color color = WHITE;
    Color endColor = {255, 255, 255, 0};
    float size = 1.0f;
    float endSize = 0.0f;
};

// A live, timer-driven emitter. Owned by ParticleSystem; the gameplay owner keeps it alive by
// calling UpdateEmitter every frame (which refreshes m_keepalive). When the owner stops updating
// — effect expired, enemy dead — the keepalive drains and Tick auto-removes it.
struct Emitter {
    EmitterDesc m_desc;
    Vector2 m_position = {};      // center anchor, refreshed by the owner each frame
    Vector2 m_baseVelocity = {};
    float m_accumulator = 0.0f;   // fractional particles carried between frames
    float m_keepalive = 0.0f;     // counts down each Tick; reset by UpdateEmitter. <=0 → removed
};

using EmitterHandle = DenseSlotMap<Emitter>::Key;

class ParticleSystem {
public:
    ParticleSystem() : m_particles(kMaxParticles) {}

    // Fire-and-forget burst: spawn desc.m_count particles at origin. baseVelocity is added to each
    // particle so effect particles on moving enemies don't lag behind their host.
    void Emit(Vector2 origin, const EmitterDesc& desc, Vector2 baseVelocity = {0, 0});

    // Continuous emission — owner registers once, then calls UpdateEmitter every frame to keep it
    // alive and follow a moving anchor. RemoveEmitter is optional (keepalive auto-cleans), but use
    // it for prompt removal. UpdateEmitter/RemoveEmitter are safe no-ops on a stale handle.
    EmitterHandle AddEmitter(const EmitterDesc& desc, Vector2 position, Vector2 baseVelocity = {0, 0});
    void UpdateEmitter(EmitterHandle h, Vector2 position, Vector2 baseVelocity = {0, 0});
    void RemoveEmitter(EmitterHandle h);

    void Tick(float dt);
    void Draw() const;
    void Clear();

private:
    static constexpr size_t kMaxParticles = 2048;
    static constexpr float kEmitterKeepalive = 0.15f; // grace period before an unrefreshed emitter dies

    // Shared spawn path for burst and continuous emission.
    void SpawnParticle(Vector2 origin, const EmitterDesc& desc, Vector2 baseVelocity);

    ObjectPool<Particle> m_particles;
    DenseSlotMap<Emitter> m_emitters;
    std::vector<EmitterHandle> m_expiredEmitters; // scratch reused each Tick to avoid per-frame allocs
};
