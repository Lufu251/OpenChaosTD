enum class TileType {
    Grass,
    Blocked,
    Core,
    Nest
};

struct Tile{
    TileType type = TileType::Grass;
    bool walkable = true;
};