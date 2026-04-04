#include <systems/world_system.hpp>
#include <world/tile.hpp>
#include <iostream>
#include <raymath.h>


 void WorldSystem::PlaceTower(int x, int y, Tower& towerTemplate, GameData& gameData){
    if(ValidateTowerPlacement(x, y, gameData)){
        Tile& tile = gameData.map.Get(x, y);

        // Add tower
        towerTemplate.m_position = gameData.map.TileToWorld(x, y);
        DenseSlotMap<Tower>::Key towerKey = gameData.towers.Insert(std::move(towerTemplate));
        
        tile.m_walkable = false;
        tile.m_buildable = false;
        tile.m_towerKey = towerKey;

        std::cout << "Tower placed x: " << x << " y: " << y << std::endl;
    }
 }

 void WorldSystem::RemoveTower(int x, int y, GameData& gameData){
    Tile& tile = gameData.map.Get(x, y);

    if(tile.m_towerKey != DenseSlotMap<Tower>::INVALID_KEY){
        // Remove tower
        gameData.towers.Erase(tile.m_towerKey);

        tile.m_walkable = true;
        tile.m_buildable = true;
        tile.m_towerKey = DenseSlotMap<Tower>::INVALID_KEY;
        std::cout << "Tower removed x: " << x << " y: " << y << std::endl;

        gameData.map.BuildFlowField();
    }
 }

bool WorldSystem::ValidateTowerPlacement(int x, int y, GameData& gameData){
    Tile& tile = gameData.map.Get(x, y);

    // Return if tile not buildable
    if(!tile.m_buildable){
        std::cout << "Tower not placed x: " << x << " y: " << y << " tile is not buildable" << std::endl;
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

void WorldSystem::UpdateEnemyMovement(GameData& gameData){
    for (auto& enemy : gameData.enemies) {
        int x, y;
        gameData.map.WorldToTile(enemy.m_position, x, y);
        std::pair<int, int> nextT = gameData.map.GetPathMesh().Get(x, y).predecessor;


        Vector2 nextP = gameData.map.TileToWorld(nextT.first, nextT.second);
        nextP += {static_cast<float>(gameData.map.GetTileSize()) /2, static_cast<float>(gameData.map.GetTileSize()) /2};
        Vector2 direction = Vector2Normalize(nextP - enemy.m_position);

        enemy.m_position += direction;
    }
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