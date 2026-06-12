#include <systems/serialization.hpp>

#include <world/map.hpp>
#include <world/tile.hpp>
#include <content/tower_factory.hpp>
#include <engine/util/file_store.hpp>

#include <cstdint>
#include <iostream>
#include <string>

// ============================================================================
// Map serialization (TOML)
// ============================================================================

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

// ============================================================================
// Save-game serialization (JSON)
// ============================================================================

bool LoadTowers(const nlohmann::json& j, DenseSlotMap<Tower>& out,
                const TowerFactory& factory, const Map& restoredMap) {
    if (!j.is_object()) return false;
    if (!j.contains("slots")    || !j["slots"].is_array())    return false;
    if (!j.contains("erase")    || !j["erase"].is_array())    return false;
    if (!j.contains("freeList") || !j["freeList"].is_array()) return false;
    if (!j.contains("values")   || !j["values"].is_array())   return false;

    const nlohmann::json& jslots  = j["slots"];
    const nlohmann::json& jvalues = j["values"];

    // Parse the sparse bookkeeping verbatim.
    std::vector<DenseSlotMap<Tower>::Slot> slots;
    slots.reserve(jslots.size());
    for (const auto& s : jslots) {
        DenseSlotMap<Tower>::Slot slot;
        slot.generation  = s.value("generation", 0u);
        slot.dense_index = s.value("dense", 0u);
        slot.occupied    = s.value("occupied", false);
        slots.push_back(slot);
    }
    std::vector<uint32_t> erase    = j["erase"].get<std::vector<uint32_t>>();
    std::vector<uint32_t> freeList = j["freeList"].get<std::vector<uint32_t>>();

    const size_t valueCount = jvalues.size();

    // Consistency checks — reject corrupt/incompatible saves before touching live state.
    if (erase.size() != valueCount) return false;

    size_t occupiedCount = 0;
    for (const auto& slot : slots)
        if (slot.occupied) ++occupiedCount;
    if (occupiedCount != valueCount) return false;

    for (size_t i = 0; i < valueCount; ++i) {
        uint32_t slotIdx = erase[i];
        if (slotIdx >= slots.size())           return false;
        if (!slots[slotIdx].occupied)          return false;
        if (slots[slotIdx].dense_index != i)   return false;
    }
    for (uint32_t f : freeList) {
        if (f >= slots.size())  return false;
        if (slots[f].occupied)  return false;
    }

    // Map dense index -> terrain modifier (occupied tiles only). The buff was baked into the
    // tower's base stats at placement (WorldSystem::PlaceTower) but never stored on the tower,
    // so it must be re-applied here before replaying upgrades to reproduce the exact stat math.
    std::vector<const TileModifier*> denseModifier(valueCount, nullptr);
    for (const Tile& tile : restoredMap.GetGrid()) {
        if (tile.m_towerKey == DenseSlotMap<Tower>::INVALID_KEY) continue;
        uint32_t slotIdx = tile.m_towerKey.index;
        if (slotIdx >= slots.size()) continue;
        const auto& slot = slots[slotIdx];
        if (!slot.occupied || slot.generation != tile.m_towerKey.generation) continue;
        if (slot.dense_index >= valueCount) continue;
        if (tile.m_modifier.Active())
            denseModifier[slot.dense_index] = &tile.m_modifier;
    }

    // Reconstruct each tower in dense order through the factory pipeline.
    std::vector<Tower> values;
    values.reserve(valueCount);
    for (size_t i = 0; i < valueCount; ++i) {
        const nlohmann::json& v = jvalues[i];
        std::string name = v.value("name", std::string{});
        auto built = factory.Create(name); // unknown tower (datapack changed) -> abort
        if (!built) return false;
        Tower tw = std::move(*built);

        // Terrain buff first (matches PlaceTower ordering), then upgrade tiers 0..level-1.
        if (const TileModifier* mod = denseModifier[i])
            tw.PatchStats(mod->m_statKey, mod->m_value, mod->m_mul);

        int level = v.value("level", 0);
        if (tw.m_upgrades) {
            int maxLevel = static_cast<int>(tw.m_upgrades->size());
            for (int l = 0; l < level && l < maxLevel; ++l)
                factory.ApplyUpgradeStats(tw, (*tw.m_upgrades)[l]);
        }

        if (v.contains("position")) tw.m_position = v["position"].get<Vector2>();
        tw.m_cooldown = v.value("cooldown", 0.0f);
        tw.m_cost     = v.value("cost", tw.m_cost);
        tw.m_level    = level;

        values.push_back(std::move(tw));
    }

    out.RawAssign(std::move(slots), std::move(values), std::move(erase), std::move(freeList));
    return true;
}
