#include <engine/systems/particle_system.hpp>

#include <raymath.h>

// Uniform random float in [-1, 1].
static float RandSigned() {
    return (float)GetRandomValue(-10000, 10000) / 10000.0f;
}

// Uniform random float in [0, 1].
static float RandUnit() {
    return (float)GetRandomValue(0, 10000) / 10000.0f;
}

// Offset of a single particle from the emitter center, per the desc's spawn shape.
static Vector2 ShapeOffset(const EmitterDesc& desc) {
    switch (desc.m_shape) {
        case SpawnShape::Line: {
            // Along the emitter angle, centred on the anchor.
            float a = desc.m_angle * DEG2RAD;
            float t = RandSigned() * desc.m_shapeSize.x * 0.5f;
            return {cosf(a) * t, sinf(a) * t};
        }
        case SpawnShape::Box:
            return {RandSigned() * desc.m_shapeSize.x * 0.5f,
                    RandSigned() * desc.m_shapeSize.y * 0.5f};
        case SpawnShape::Circle: {
            float a = RandUnit() * 2.0f * PI;
            float r = RandUnit() * desc.m_shapeRadius;
            return {cosf(a) * r, sinf(a) * r};
        }
        case SpawnShape::Ring: {
            float a = RandUnit() * 2.0f * PI;
            return {cosf(a) * desc.m_shapeRadius, sinf(a) * desc.m_shapeRadius};
        }
        case SpawnShape::Point:
        default:
            return {0, 0};
    }
}

void ParticleSystem::SpawnParticle(Vector2 origin, const EmitterDesc& desc, Vector2 baseVelocity) {
    Particle* p = m_particles.Acquire();
    if (!p) return; // pool at capacity — drop silently

    // Direction within the spread cone around the base angle.
    float baseAngle = desc.m_angle * DEG2RAD;
    float halfArc   = desc.m_spread * 0.5f * DEG2RAD;
    float dirAngle  = baseAngle + RandSigned() * halfArc;

    float speed = std::max(0.0f, desc.m_speed + RandSigned() * desc.m_speedVariance);
    float life  = std::max(0.01f, desc.m_lifetime);

    Vector2 offset = ShapeOffset(desc);

    p->center = origin;
    p->position = Vector2Add(origin, offset);
    p->velocity = {cosf(dirAngle) * speed + baseVelocity.x, sinf(dirAngle) * speed + baseVelocity.y};
    p->seedDir = {cosf(dirAngle), sinf(dirAngle)};
    p->radialSpeed = desc.m_radialSpeed;
    p->tangentialSpeed = desc.m_tangentialSpeed;
    p->lifetime = life;
    p->maxLifetime = life;
    p->color = desc.m_color;
    p->endColor = desc.m_endColor;
    p->size = desc.m_size;
    p->endSize = desc.m_endSize;
}

void ParticleSystem::Emit(Vector2 origin, const EmitterDesc& desc, Vector2 baseVelocity) {
    for (int i = 0; i < desc.m_count; i++)
        SpawnParticle(origin, desc, baseVelocity);
}

EmitterHandle ParticleSystem::AddEmitter(const EmitterDesc& desc, Vector2 position, Vector2 baseVelocity) {
    Emitter e;
    e.m_desc = desc;
    e.m_position = position;
    e.m_baseVelocity = baseVelocity;
    e.m_keepalive = kEmitterKeepalive;
    return m_emitters.Insert(std::move(e));
}

void ParticleSystem::UpdateEmitter(EmitterHandle h, Vector2 position, Vector2 baseVelocity) {
    if (Emitter* e = m_emitters.Get(h)) {
        e->m_position = position;
        e->m_baseVelocity = baseVelocity;
        e->m_keepalive = kEmitterKeepalive;
    }
}

void ParticleSystem::RemoveEmitter(EmitterHandle h) {
    m_emitters.Erase(h);
}

void ParticleSystem::Tick(float dt) {
    // Advance live emitters: spawn on their timer, then collect any that have gone unrefreshed.
    m_expiredEmitters.clear();
    for (Emitter& e : m_emitters) {
        e.m_keepalive -= dt;
        if (e.m_keepalive <= 0.0f) {
            m_expiredEmitters.push_back(m_emitters.KeyOf(&e));
            continue;
        }
        if (e.m_desc.m_emitRate <= 0.0f) continue;

        e.m_accumulator += e.m_desc.m_emitRate * dt;
        while (e.m_accumulator >= 1.0f) {
            for (int i = 0; i < std::max(1, e.m_desc.m_count); i++)
                SpawnParticle(e.m_position, e.m_desc, e.m_baseVelocity);
            e.m_accumulator -= 1.0f;
        }
    }
    for (EmitterHandle h : m_expiredEmitters)
        m_emitters.Erase(h);

    // Integrate particles with the unified motion model (linear + radial + tangential).
    size_t i = 0;
    while (i < m_particles.Size()) {
        Particle& p = m_particles[i];

        Vector2 toP = Vector2Subtract(p.position, p.center);
        float len = Vector2Length(toP);
        Vector2 radial = (len > 0.0001f) ? Vector2Scale(toP, 1.0f / len) : p.seedDir;
        Vector2 tang = {-radial.y, radial.x};

        Vector2 v = Vector2Add(p.velocity,
                    Vector2Add(Vector2Scale(radial, p.radialSpeed),
                               Vector2Scale(tang, p.tangentialSpeed)));
        p.position = Vector2Add(p.position, Vector2Scale(v, dt));
        p.lifetime -= dt;

        if (p.lifetime <= 0.0f) {
            m_particles.ReleaseAt(i);
            // i unchanged — the former last particle is now at this index
        } else {
            i++;
        }
    }
}

void ParticleSystem::Draw() const {
    for (const auto& p : m_particles) {
        float t = p.lifetime / p.maxLifetime;
        Color c = ColorLerp(p.endColor, p.color, t);
        float s = Lerp(p.endSize, p.size, t);

        DrawCircleV(p.position, s, c);
    }
}

void ParticleSystem::Clear() {
    m_particles.Clear();
    m_emitters.Clear();
}
