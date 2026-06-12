#include <app/game_data.hpp>
#include <systems/serialization.hpp>
#include <engine/util/file_store.hpp>
#include <iostream>

// Bumped when the on-disk save schema changes incompatibly; older versions are rejected.
static constexpr int kSaveVersion = 1;

void GameData::Load(FileStore& fileStore, const std::string& dataDir) {
    std::string path = dataDir + "/gameplay.toml";
    if (!fileStore.Exists(path))
        return;

    auto data = fileStore.LoadToml(path);
    m_startingLives  = data["startingLives"].value_or(m_startingLives);
    m_startingGold   = data["startingGold"].value_or(m_startingGold);
    m_sellRefundRate = data["sellRefundRate"].value_or(m_sellRefundRate);
    m_autoSpawnDelay = data["autoSpawnDelay"].value_or(m_autoSpawnDelay);

    m_lives = m_startingLives;
    m_gold = m_startingGold;

    std::string mapPath = dataDir + "/map_generation.toml";
    if (fileStore.Exists(mapPath)) {
        auto mapData = fileStore.LoadToml(mapPath);
        if (const toml::table* mp = mapData["map"].as_table()) {
            m_mapGenCfg.minCluster       = (*mp)["minCluster"].value_or(m_mapGenCfg.minCluster);
            m_mapGenCfg.maxCluster       = (*mp)["maxCluster"].value_or(m_mapGenCfg.maxCluster);
            m_mapGenCfg.seedTries        = (*mp)["seedTries"].value_or(m_mapGenCfg.seedTries);
            m_mapGenCfg.growTries        = (*mp)["growTries"].value_or(m_mapGenCfg.growTries);
            m_mapGenCfg.tilesPerBuffTile = (*mp)["tilesPerBuffTile"].value_or(m_mapGenCfg.tilesPerBuffTile);
        }
    }
}

void GameData::Reset() {
    m_lives = m_startingLives;
    m_gold = m_startingGold;
    m_victory = false;
    m_waveNumber = 0;
    m_waveActive = false;
    m_map = Map();
    m_towers = DenseSlotMap<Tower>();
    m_enemies = DenseSlotMap<Enemy>();
    m_attacks.clear();
    // m_selectedMapDir is intentionally left untouched: it is the cross-restart map
    // choice, not per-playthrough state, so Restart / Play Again replay the same map.
}

void GameData::SaveState(FileStore& fileStore, const std::string& path, const std::string& datapack) const {
    nlohmann::json j;
    j["version"]    = kSaveVersion;
    j["datapack"]   = datapack; // which pack's factory this save's tower names belong to
    j["lives"]      = m_lives;
    j["gold"]       = m_gold;
    j["waveNumber"] = m_waveNumber;
    j["victory"]    = m_victory;

    // Map: only the authoritative geometry. The path mesh and waypoint vectors are derived
    // and get rebuilt by BuildPathMesh on load. Enemies/attacks don't exist between waves.
    j["map"]["tileSize"] = m_map.GetTileSize();
    j["map"]["core"]     = m_map.GetCore();
    j["map"]["nests"]    = m_map.GetNests();
    j["map"]["grid"]     = m_map.GetGrid();

    j["towers"] = SaveTowers(m_towers);

    fileStore.SaveJson(path, j);
}

bool GameData::LoadState(FileStore& fileStore, const std::string& path, const TowerFactory& factory) {
    nlohmann::json j = fileStore.LoadJson(path); // {} on a missing file or parse error
    if (!j.is_object() || !j.contains("map") || !j.contains("towers")) return false;
    if (j.value("version", 0) != kSaveVersion) return false;

    // Build everything into locals first; only commit to *this once the whole load succeeds,
    // so a corrupt save can never leave an in-progress game half-overwritten.
    try {
        const nlohmann::json& jm = j.at("map");

        Grid2D<Tile> grid = jm.at("grid").get<Grid2D<Tile>>();
        int tileSize = jm.value("tileSize", 32);
        auto core    = jm.at("core").get<std::pair<int, int>>();
        auto nests   = jm.at("nests").get<std::vector<std::pair<int, int>>>();

        Map tmpMap;
        tmpMap.RestoreFromSave(std::move(grid), tileSize, core, std::move(nests));

        DenseSlotMap<Tower> towers;
        if (!LoadTowers(j.at("towers"), towers, factory, tmpMap)) return false;

        // Commit — no throwing operations past this point.
        m_map        = std::move(tmpMap);
        m_towers     = std::move(towers);
        m_lives      = j.value("lives", m_startingLives);
        m_gold       = j.value("gold", m_startingGold);
        m_waveNumber = j.value("waveNumber", 0);
        m_victory    = j.value("victory", false);
        m_waveActive = false;
        m_enemies.Clear();
        m_attacks.clear();
        return true;
    } catch (const std::exception& e) {
        // Malformed shape/type anywhere in the parse block. Keep the one catch here at the
        // json parse edge, but surface the reason instead of swallowing it silently.
        std::cerr << "GameData::LoadState: rejecting corrupt save: " << e.what() << "\n";
        return false;
    }
}
