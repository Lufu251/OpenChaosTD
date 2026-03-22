#pragma once

#include <game.hpp>
#include <world/tower.hpp>

class WorldSystem{
public:
    void PlaceTower(Tower& tower, Game& game, Camera2D& camera);
    void RemoveTower(Map& map);

    void GenerateMap(Map& map, int x, int y);

private:
    bool PlaceTowerAllowed(int x, int y, Game& game);
};