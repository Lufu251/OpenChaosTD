#pragma once

enum class TileType {
    Grass,
    Rock,
    Core,
    Nest
};

struct Tile{
    TileType m_type = TileType::Grass;
    bool m_walkable = true;
    bool m_buildable = true;
};