#pragma once

// Serialization for maps (TOML map files) and persisted game state (JSON). Both live in the
// systems layer so the world data types and the engine containers (Grid2D, DenseSlotMap) stay
// free of any toml/json dependency: they expose generic raw accessors and this is the single
// place that reaches into them — through the namespaced map API and non-intrusive ADL
// to_json/from_json — to (de)serialize.

#include <vector>
#include <string>

#include <raylib.h>
#include <toml++/toml.hpp>
#include <nlohmann/json.hpp>

#include <world/tile.hpp>  // Tile + DenseSlotMap<Tower>::Key
#include <world/tower.hpp> // Tower (move-only) — serialized by value below
#include <engine/lib/grid2d.hpp>
#include <engine/lib/dense_slotmap.hpp>

class Map;
class FileStore;
class TowerFactory;
class TileFactory;

// ============================================================================
// Map serialization (TOML)
// ============================================================================

// Reads and writes a custom map as "<mapDir>/map.toml". Kept out of the Map class
// (which is included almost everywhere) so toml++ stays a serialization-only
// dependency — mirroring how EmitterPresets keeps toml off of EmitterDesc.
namespace MapSerialization {

// Authoring metadata stored alongside the grid; not part of Map itself.
struct MapMeta {
    std::string m_name;
    std::string m_description;
};

// Build the full TOML table for a map + metadata (inverse of Load).
toml::table BuildMapTable(const Map& map, const MapMeta& meta);

// Write "<mapDir>/map.toml". Returns false if the FileStore write fails.
bool Save(FileStore& fileStore, const std::string& mapDir, const Map& map, const MapMeta& meta);

// Read "<mapDir>/map.toml" and reconstruct outMap (grid + geometry + path mesh) and
// outMeta. Returns false on a missing/malformed file.
bool Load(FileStore& fileStore, const std::string& mapDir, Map& outMap, MapMeta& outMeta,
          const TileFactory& tileFactory, const std::string& groundId);

} // namespace MapSerialization

// ============================================================================
// Save-game serialization (JSON)
// ============================================================================

// --- ADL serializers for value types (global namespace so ADL finds them) ---

inline void to_json(nlohmann::json& j, const Vector2& v) {
    j = nlohmann::json{{"x", v.x}, {"y", v.y}};
}
inline void from_json(const nlohmann::json& j, Vector2& v) {
    v.x = j.at("x").get<float>();
    v.y = j.at("y").get<float>();
}

inline void to_json(nlohmann::json& j, const TileModifier& m) {
    j = nlohmann::json{{"statKey", m.m_statKey}, {"value", m.m_value}, {"mul", m.m_mul}};
}
inline void from_json(const nlohmann::json& j, TileModifier& m) {
    m.m_statKey = j.value("statKey", std::string{});
    m.m_value   = j.value("value", 0.0f);
    m.m_mul     = j.value("mul", false);
}

inline void to_json(nlohmann::json& j, const Tile& t) {
    j = nlohmann::json{
        {"tileId", t.m_tileId},
        {"walkable", t.m_walkable},
        {"buildable", t.m_buildable},
        {"textureIndex", t.m_textureIndex},
        {"modifier", t.m_modifier},
        {"towerKey", {{"index", t.m_towerKey.index}, {"generation", t.m_towerKey.generation}}},
    };
}
inline void from_json(const nlohmann::json& j, Tile& t) {
    // Backward compat: old saves used integer "type" (0=Grass, 1=Rock, 2=Core, 3=Nest, 4=Buff).
    if (j.contains("type") && j["type"].is_number_integer() && !j.contains("tileId")) {
        static const char* kLegacyMap[] = {"grass", "rock", "core", "nest", "buff"};
        int legacy = j.value("type", 0);
        int idx = (legacy >= 0 && legacy < 5) ? legacy : 0;
        t.m_tileId = kLegacyMap[idx];
    } else {
        t.m_tileId = j.value("tileId", std::string("grass"));
    }
    t.m_walkable     = j.value("walkable", true);
    t.m_buildable    = j.value("buildable", true);
    t.m_textureIndex = j.value("textureIndex", 0);
    t.m_modifier     = j.value("modifier", TileModifier{});
    const auto& k = j.at("towerKey");
    t.m_towerKey  = { k.at("index").get<uint32_t>(), k.at("generation").get<uint32_t>() };
}

// Grid2D<T> — width/height plus the flat row-major cell vector. Generic over any
// serializable cell type; never includes game logic.
template<class T>
void to_json(nlohmann::json& j, const Grid2D<T>& g) {
    j = nlohmann::json{{"width", g.GetWidth()}, {"height", g.GetHeight()}, {"data", g.GetVector()}};
}
template<class T>
void from_json(const nlohmann::json& j, Grid2D<T>& g) {
    int w = j.at("width").get<int>();
    int h = j.at("height").get<int>();
    // Reject non-positive dimensions before the product check: a 0x0 grid would otherwise pass
    // (0 == empty data) and build a degenerate grid / trip Grid2D::Resize's positivity assert.
    if (w <= 0 || h <= 0)
        throw std::runtime_error("Grid2D: width and height must be positive");
    auto data = j.at("data").template get<std::vector<T>>();
    if (static_cast<size_t>(w) * static_cast<size_t>(h) != data.size())
        throw std::runtime_error("Grid2D: data size does not match width*height");
    g.Resize(w, h);
    g.GetVector() = std::move(data);
}

// --- Tower / DenseSlotMap<Tower> ---
// Towers are serialized identity-only (metadata); their polymorphic module lists are rebuilt
// through TowerFactory on load. The one piece of per-tower module state that isn't reproducible
// from the factory + upgrade replay is the player's chosen targeting mode, so that single override
// is persisted and re-applied on load. The slotmap's sparse bookkeeping is persisted verbatim so
// DenseSlotMap<Tower>::Key handles (e.g. Tile::m_towerKey) survive.

inline nlohmann::json SaveTower(const Tower& t) {
    nlohmann::json j{
        {"name", t.m_name},
        {"position", t.m_position},
        {"level", t.m_level},
        {"cooldown", t.m_cooldown},
        {"cost", t.m_cost},
    };
    // Wall towers have no AttackModule, so guard before recording the targeting override.
    if (const AttackModule* atk = t.GetAttack())
        j["targeting"] = static_cast<int>(atk->m_targetingMode);
    return j;
}

inline nlohmann::json SaveTowers(const DenseSlotMap<Tower>& towers) {
    nlohmann::json slots = nlohmann::json::array();
    for (const auto& s : towers.RawSlots())
        slots.push_back({{"generation", s.generation}, {"dense", s.dense_index}, {"occupied", s.occupied}});

    nlohmann::json values = nlohmann::json::array();
    for (const auto& t : towers.RawValues())
        values.push_back(SaveTower(t));

    return nlohmann::json{
        {"slots", slots},
        {"erase", towers.RawErase()},
        {"freeList", towers.RawFreeList()},
        {"values", values},
    };
}

// Rebuild the tower slotmap from JSON. Returns false (leaving `out` unspecified) on any
// structural inconsistency or unknown tower name, so a corrupt save never half-loads.
// `restoredMap` supplies the tiles whose terrain modifiers must be re-applied to towers.
bool LoadTowers(const nlohmann::json& j, DenseSlotMap<Tower>& out,
                const TowerFactory& factory, const Map& restoredMap);
