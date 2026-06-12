#pragma once

#include <world/map.hpp>
#include <world/tower.hpp>
#include <world/enemy.hpp>
#include <world/combat.hpp>
#include <engine/lib/dense_slotmap.hpp>
#include <systems/map_generator.hpp>
#include <vector>

class FileStore;
class TowerFactory;

struct GameData {
    // Gameplay config — loaded from gameplay.toml, not reset between games
    int m_startingLives = 20;
    int m_startingGold = 150;
    float m_sellRefundRate = 0.5f;
    float m_autoSpawnDelay = 3.0f; // seconds between waves when auto-spawn is on
    MapGenCfg m_mapGenCfg;

    int m_lives = 20;
    int m_gold = 150;
    bool m_victory = false;

    // Wave state
    int   m_waveNumber = 0;
    bool  m_waveActive = false;
    Map m_map;
    DenseSlotMap<Tower> m_towers;
    DenseSlotMap<Enemy> m_enemies;
    std::vector<Attack> m_attacks;

    // Which map a fresh game should build: empty = procedural generation, otherwise the
    // relative dir of a custom map (e.g. "datapacks/default/maps/test"). Set by the map
    // selection screen; deliberately survives Reset() so Restart / Play Again replay the
    // same map. Not part of the save (a save embeds its own map geometry).
    std::string m_selectedMapDir;

    // dataDir is the active datapack's data directory (relative to the project root).
    void Load(FileStore& fileStore, const std::string& dataDir);
    void Reset();

    // Persist/restore an in-progress game to/from a JSON file. Saving is only valid between
    // waves, so enemies and active attacks are intentionally not part of the state. LoadState
    // reconstructs towers through `factory` and commits atomically: on any error it returns
    // false and leaves the current game untouched.
    void SaveState(FileStore& fileStore, const std::string& path, const std::string& datapack) const;
    bool LoadState(FileStore& fileStore, const std::string& path, const TowerFactory& factory);
};
