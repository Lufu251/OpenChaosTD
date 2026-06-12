#include <world/map_serialization.hpp>

#include <world/map.hpp>
#include <world/tile.hpp>
#include <engine/util/file_store.hpp>

#include <cstdint>
#include <iostream>
#include <string>

namespace {

const char* TileTypeName(TileType type) {
    switch (type) {
        case TileType::Rock: return "Rock";
        case TileType::Core: return "Core";
        case TileType::Nest: return "Nest";
        case TileType::Buff: return "Buff";
        default:             return "Grass";
    }
}

TileType ParseTileType(const std::string& s) {
    if (s == "Rock") return TileType::Rock;
    if (s == "Core") return TileType::Core;
    if (s == "Nest") return TileType::Nest;
    if (s == "Buff") return TileType::Buff;
    return TileType::Grass;
}

} // namespace

namespace MapSerialization {

toml::table BuildMapTable(const Map& map, const MapMeta& meta) {
    int cols = map.GetCols();
    int rows = map.GetRows();

    toml::table root;

    // [meta]
    root.insert("meta", toml::table{
        {"name", meta.m_name},
        {"description", meta.m_description},
    });

    // [dimensions]
    root.insert("dimensions", toml::table{
        {"cols", static_cast<int64_t>(cols)},
        {"rows", static_cast<int64_t>(rows)},
        {"tileSize", static_cast<int64_t>(map.GetTileSize())},
    });

    // [geometry] — redundant with the tile types but keeps the file self-describing.
    toml::array core{static_cast<int64_t>(map.GetCore().first),
                     static_cast<int64_t>(map.GetCore().second)};
    toml::array nests;
    for (const auto& nest : map.GetNests())
        nests.push_back(toml::array{static_cast<int64_t>(nest.first),
                                    static_cast<int64_t>(nest.second)});
    root.insert("geometry", toml::table{
        {"core", std::move(core)},
        {"nests", std::move(nests)},
    });

    // [[tiles]] — row-major (index = y * cols + x), one inline table per cell.
    toml::array tiles;
    for (int y = 0; y < rows; y++) {
        for (int x = 0; x < cols; x++) {
            const Tile& tile = map.Get(x, y);
            toml::table t{
                {"type", TileTypeName(tile.m_type)},
                {"walkable", tile.m_walkable},
                {"buildable", tile.m_buildable},
            };
            if (tile.m_modifier.Active()) {
                t.insert("statKey", tile.m_modifier.m_statKey);
                t.insert("value", std::stod(FormatFloat(tile.m_modifier.m_value)));
                t.insert("mul", tile.m_modifier.m_mul);
            }
            tiles.push_back(std::move(t));
        }
    }
    root.insert("tiles", std::move(tiles));

    return root;
}

bool Save(FileStore& fileStore, const std::string& mapDir, const Map& map, const MapMeta& meta) {
    toml::table table = BuildMapTable(map, meta);
    fileStore.SaveToml(mapDir + "/map.toml", table);
    return true;
}

bool Load(FileStore& fileStore, const std::string& mapDir, Map& outMap, MapMeta& outMeta) {
    toml::table table = fileStore.LoadToml(mapDir + "/map.toml");
    if (table.empty()) {
        std::cerr << "MapSerialization: failed to load '" << mapDir << "/map.toml'\n";
        return false;
    }

    int cols = table["dimensions"]["cols"].value_or(0);
    int rows = table["dimensions"]["rows"].value_or(0);
    if (cols <= 0 || rows <= 0) {
        std::cerr << "MapSerialization: invalid dimensions in '" << mapDir << "/map.toml'\n";
        return false;
    }

    outMeta.m_name        = table["meta"]["name"].value_or(std::string{});
    outMeta.m_description = table["meta"]["description"].value_or(std::string{});

    outMap.Create(cols, rows);

    // Paint each tile from the saved row-major array; geometry (core/nests) and the
    // path mesh are re-derived afterwards from the painted types.
    const toml::array* tiles = table["tiles"].as_array();
    if (tiles && static_cast<int>(tiles->size()) == cols * rows) {
        for (int i = 0; i < cols * rows; i++) {
            const toml::table* t = tiles->get(i)->as_table();
            if (!t) continue;
            int x = i % cols;
            int y = i / cols;
            Tile& tile = outMap.Get(x, y);
            tile.m_type = ParseTileType((*t)["type"].value_or(std::string("Grass")));
            tile.m_walkable = (*t)["walkable"].value_or(true);
            tile.m_buildable = (*t)["buildable"].value_or(true);
            if (tile.m_type == TileType::Buff) {
                tile.m_modifier = {
                    (*t)["statKey"].value_or(std::string{}),
                    (*t)["value"].value_or(0.0f),
                    (*t)["mul"].value_or(false),
                };
            }
        }
    }

    outMap.RebuildGeometryFromGrid();
    return true;
}

} // namespace MapSerialization
