#pragma once

#include <world/map.hpp>
#include <entities/tower.hpp>
#include <entities/enemy.hpp>
#include <core/asset_manager.hpp>

class RenderSystem{
public:
    Camera2D camera;
    
    // Draw calls
    void DrawMap(const Map& map, AssetManager& assets);
    void DrawTowers(const std::vector<Tower>& towers, AssetManager& assets);
    void DrawEnemys(const std::vector<Enemy>& enemies, AssetManager& assets);
    // void DrawProjectiles

private:
    
};