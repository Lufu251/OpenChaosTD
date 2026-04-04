#pragma once

#include <world/map.hpp>
#include <core/asset_manager.hpp>
#include <core/input_manager.hpp>
#include <core/renderer.hpp>
#include <world/tower.hpp>
#include <world/enemy.hpp>
#include <lib/dense_slotmap.hpp>

class RenderSystem{
public:
    // Draw calls
    void DrawMap(const Map& map, AssetManager& assets);
    void DebugDrawMap(const Map& Map);
    void DrawTowers(const DenseSlotMap<Tower>& towers, AssetManager& assets);
    void DrawEnemies(const DenseSlotMap<Enemy>& enemies, AssetManager& assets);
    
    void CenterCamera(Map& map, Renderer& renderer);
    void ControlCamera(InputManager& input);

    // Access
    const Camera2D& GetCamera(){return camera;}

private:
    Vector2 mousePositionLast;

    Camera2D camera;
    int zoomIndex = 1;
    float zoomLevel[4] = {0.5, 1, 2, 4};
};