#pragma once

#include <world/map.hpp>
#include <core/asset_manager.hpp>
#include <core/input_manager.hpp>
#include <core/renderer.hpp>
#include <world/tower.hpp>
#include <vector>

class RenderSystem{
public:
    // Draw calls
    void DrawMap(const Map& map, AssetManager& assets);
    void DebugDrawMap(const Map& Map);
    void DrawTower(const std::vector<Tower>& towers, AssetManager& assets);
    
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