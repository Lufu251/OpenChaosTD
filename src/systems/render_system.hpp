#pragma once

#include <world/map.hpp>
#include <engine/core/resources.hpp>
#include <engine/core/input.hpp>
#include <engine/core/screen.hpp>
#include <world/tower.hpp>
#include <world/enemy.hpp>
#include <world/combat.hpp>
#include <engine/lib/dense_slotmap.hpp>
#include <vector>

class FileStore;

class RenderSystem{
public:
    // Draw calls
    void DrawMap(const Map& map, Resources& assets);
    void DrawPaths(const Map& map);
    void DebugDrawMap(const Map& Map);
    void DebugDrawEnemies(const DenseSlotMap<Enemy>& enemies);
    void DrawTowers(const DenseSlotMap<Tower>& towers, Resources& assets);
    void DrawTowerRange(Vector2 position, float radius, Color color);
    void DrawRangeIndicator(DenseSlotMap<Tower>::Key selectedKey, const Map& map, const DenseSlotMap<Tower>& towers, Vector2 mouseWorld);
    void DrawGhostTower(Vector2 position, float radius, Texture2D& texture);
    void DrawEnemies(const DenseSlotMap<Enemy>& enemies, Resources& assets);
    void DrawAttacks(const std::vector<Attack>& attacks);

    void Load(FileStore& fileStore); // reads config/render.toml; safe to call before first use

    void CenterCamera(Map& map, Screen& renderer);
    void CenterCamera(Map& map, Rectangle viewport);
    void ControlCamera(float& dt, Input& input);

    // Access
    const Camera2D& GetCamera(){return m_camera;}

private:
    Vector2 m_mousePositionLast;

    Camera2D m_camera{}; // zero-init: rotation/zoom/etc. start defined, never indeterminate
    int m_zoomIndex = 1;
    std::vector<float> m_zoomLevels = {0.5f, 1.0f, 2.0f, 4.0f};

    // Health bar visual config (loaded from config/render.toml)
    float m_hbWidth    = 20.0f;
    float m_hbHeight   = 4.0f;
    float m_hbPadding  = 1.0f;
    float m_hbRound    = 0.2f;
    int   m_hbSegs     = 8;
    Color m_hbBgColor  = {40, 40, 40, 255};

    void DrawHealthBar(Vector2 worldPos, float current, float max);
};