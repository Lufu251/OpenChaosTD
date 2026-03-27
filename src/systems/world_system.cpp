#include <systems/world_system.hpp>
#include <world/tile.hpp>
#include <iostream>


 void WorldSystem::PlaceTower(Tower& tower, Game& game, Camera2D& camera){
    int x, y;
    if(game.GetGameData().map.WorldToTile(GetScreenToWorld2D(game.GetInput().GetMousePosition(), camera), x, y) && PlaceTowerAllowed(x, y, game)){
        // Add tower
        tower.m_position = game.GetGameData().map.TileToWorld(x, y);
        game.GetGameData().towers.push_back(std::move(tower));

        Tile& tile = game.GetGameData().map.Get(x, y);
        tile.m_walkable = false;
        tile.m_buildable = false;
        std::cout << "Placed Tower" << std::endl;
    }
 }

bool WorldSystem::PlaceTowerAllowed(int x, int y, Game& game){
    Tile tile = game.GetGameData().map.Get(x, y);

    if(tile.m_buildable){
        return true;
    }

    return false;
}


void WorldSystem::GenerateMap(Map& map, int x, int y){
    map.Create(x, y);

    int xmid = (map.GetCols() -1) /2;
    int ymid = (map.GetRows() -1) /2;
    for(int x=0; x < map.GetCols(); x++) {
        for(int y=0; y < map.GetRows(); y++) {
            
            //Add Nest at the top
            if(y == 1 && x == xmid) map.AddNest(x, y);

            // Add Rock formation
            Tile rockTile;
            rockTile.m_type = TileType::Rock;
            rockTile.m_buildable = false;
            rockTile.m_walkable = false;

            if(y == ymid && x < map.GetCols() -3) map.Get(x, y) = std::move(rockTile);

            //Place Core at the bottom
            if(y == map.GetRows() -2 && x == xmid) map.SetCore(x, y);
        }
    }
}