#pragma once

#include <game.hpp>
#include <world/tower.hpp>

class WorldSystem{
public:
    void PlaceTower(int x, int y, Tower& towerTemplate, GameData& gameData);
    void RemoveTower(int x, int y, GameData& gameData);

    void GenerateMap(Map& map, int x, int y);

private:
    bool ValidateTowerPlacement(int x, int y, GameData& gameData);
};