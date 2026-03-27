#pragma once

#include <world/map.hpp>
#include <core/asset_manager.hpp>
#include <world/tower.hpp>
#include <vector>

class RenderSystem{
public:
    Camera2D camera;
    
    // Draw calls
    void DrawMap(const Map& map, AssetManager& assets);
    void DrawTower(const std::vector<Tower>& towers, AssetManager& assets);

private:
    
};