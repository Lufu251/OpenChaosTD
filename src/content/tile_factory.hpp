#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include <world/tile.hpp>

class FileStore;

// Registry of tile type definitions loaded from a datapack's tiles.toml.
// TileFactory is a pure data store (no object construction), mirroring the
// Load/Clear/Get pattern shared by TowerFactory and EnemyFactory.
class TileFactory {
public:
    struct TileDef {
        std::string id;
        bool walkable = true;
        bool buildable = true;
        std::vector<std::string> textures; // one randomly picked per tile instance
        TileModifier modifier;             // embedded terrain buff (inactive by default)
    };

    void Load(FileStore& fileStore, const std::string& dataDir);
    void Clear();

    bool Has(const std::string& id) const;
    const TileDef& Get(const std::string& id) const;

    // Returns IDs of all tiles with an active modifier (buff tiles), in registration order.
    std::vector<std::string> GetBuffIds() const;

    const std::vector<std::string>& GetIds() const { return m_order; }

private:
    std::unordered_map<std::string, TileDef> m_defs; // id -> TileDef
    std::vector<std::string> m_order;                 // insertion order
};
