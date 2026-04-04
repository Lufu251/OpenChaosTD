#pragma once

#include <world/tower.hpp>
#include <lib/dense_slotmap.hpp>

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

    DenseSlotMap<Tower>::Key m_towerKey = DenseSlotMap<Tower>::INVALID_KEY;
};