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

        gameData.map.BuildPathMesh();
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
    gameData.map.BuildPathMesh();
    if(!gameData.map.ValidatePathMesh()){
        tile.m_walkable = true;
        gameData.map.BuildPathMesh();
        std::cout << "Tower not placed x: " << x << " y: " << y << " blocks path to core" << std::endl;
        return false;
    }

    // If nothing fails allow tower placement
    return true;
}

void WorldSystem::UpdateEnemyTargets(GameData& gameData){
    for (auto& enemy : gameData.enemies) {
        // Check if no target is assigned
        if(enemy.m_target.x == MAXFLOAT && enemy.m_target.x == MAXFLOAT){
            // Current tile
            int cX, cY;
            gameData.map.WorldToTile(enemy.m_position, cX, cY);
            
            // Next tile
            std::pair<int, int> n = gameData.map.GetPathMesh().Get(cX, cY).predecessor;

            // Set target
            enemy.m_target = gameData.map.TileToWorld(n.first, n.second);
            enemy.m_target += {static_cast<float>(gameData.map.GetTileSize()) /2, static_cast<float>(gameData.map.GetTileSize()) /2};
        }
    }
}

void WorldSystem::UpdateEnemyPosition(float& dt, GameData& gameData){
    for (auto& enemy : gameData.enemies) {
        Vector2 distance = enemy.m_target - enemy.m_position; // Distance vector to target
        Vector2 movement = Vector2Normalize(distance) * enemy.m_speed * dt; // Movement vector

        // Add remaining distance from last iteration to movement
        if(enemy.m_remainingDistance > 0){
            movement += Vector2Normalize(movement) * (Vector2Length(movement) + enemy.m_remainingDistance);
            enemy.m_remainingDistance = 0;
        }
        
        // Check if target is reached
        float delta = Vector2Length(movement) - Vector2Length(distance); // Positiv if overshooting
        if(delta > 0){
            // Is overshooting
            enemy.m_target = {MAXFLOAT, MAXFLOAT}; // Reset target
            enemy.m_remainingDistance = delta; // Save the delta for next movement, to prevent loosing it
            movement = distance; // Set movement to the exact distance to target
        }

        // Move enemy
        enemy.m_position += movement;
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