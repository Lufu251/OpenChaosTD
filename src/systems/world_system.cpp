#include <systems/world_system.hpp>
#include <world/tile.hpp>
#include <iostream>


 void WorldSystem::PlaceTower(int x, int y, Tower& towerTemplate, GameData& gameData){
    if(ValidateTowerPlacement(x, y, gameData)){
        // Add tower
        towerTemplate.m_position = gameData.map.TileToWorld(x, y);
        gameData.towers.push_back(std::move(towerTemplate));

        Tile& tile = gameData.map.Get(x, y);
        tile.m_walkable = false;
        tile.m_buildable = false;

        std::cout << "Tower placed x: " << x << " y: " << y << std::endl;
    }
 }

 void WorldSystem::RemoveTower(int x, int y, GameData& gameData){

 }

bool WorldSystem::ValidateTowerPlacement(int x, int y, GameData& gameData){
    Tile& tile = gameData.map.Get(x, y);

    // Return if tile not buildable
    if(!tile.m_buildable){
        std::cout << "Tower not placed x: " << x << " y: " << y << "  tile is not buildable" << std::endl;
        return false;
    }

    // Check if paths are still valid after tower placement
    tile.m_walkable = false;
    gameData.map.BuildFlowField();
    if(!gameData.map.ValidatePaths()){
        tile.m_walkable = true;
        gameData.map.BuildFlowField();
        std::cout << "Tower not placed x: " << x << " y: " << y << " blocks path to core" << std::endl;
        return false;
    }


    // If nothing fails allow tower placement
    return true;
}


void WorldSystem::GenerateMap(Map& map, int x, int y){
    map.Create(x, y);

    int xmid = (map.GetCols() -1) /2;
    int ymid = (map.GetRows() -1) /2;
    for(int x=0; x < map.GetCols(); x++) {
        for(int y=0; y < map.GetRows(); y++) {
            
            //Add Nest at the top
            if(y == 1 && x == xmid) map.AddNest(x, y);
            if(y == 1 && x == 1) map.AddNest(x, y);
            if(y == 1 && x == map.GetCols()-2) map.AddNest(x, y);

            //Place Core at the bottom
            if(y == map.GetRows() -2 && x == xmid) map.SetCore(x, y);

            // Add Rock formation
            Tile rockTile;
            rockTile.m_type = TileType::Rock;
            rockTile.m_buildable = false;
            rockTile.m_walkable = false;

            if(y == ymid && x < map.GetCols() -3) map.Get(x, y) = std::move(rockTile);

            
        }
    }
}