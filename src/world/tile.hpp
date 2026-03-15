enum class TileType {
    Grass,
    Boulder
};

struct Tile{
    TileType type = TileType::Grass;
};