#pragma once

#include <world/map.hpp>
#include <engine/core/resources.hpp>
#include <engine/core/input.hpp>
#include <engine/core/screen.hpp>
#include <world/tower.hpp>
#include <world/enemy.hpp>
#include <world/combat.hpp>
#include <engine/lib/dense_slotmap.hpp>

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

    void CenterCamera(Map& map, Screen& renderer);
    void CenterCamera(Map& map, Rectangle viewport);
    void ControlCamera(float& dt, Input& input);

    // Access
    const Camera2D& GetCamera(){return m_camera;}

private:
    Vector2 m_mousePositionLast;

    Camera2D m_camera{}; // zero-init: rotation/zoom/etc. start defined, never indeterminate
    int m_zoomIndex = 1;
    float m_zoomLevel[4] = {0.5, 1, 2, 4};

    void DrawHealthBar(Vector2 worldPos, float current, float max, float width, float height);
};