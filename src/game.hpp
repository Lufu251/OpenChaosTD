#pragma once

#include <raylib.h>
#include <states/game_state.hpp>
#include <core/asset_manager.hpp>
#include <core/renderer.hpp>
#include <core/input_manager.hpp>
#include <core/jsonio.hpp>
#include <core/performance_monitor.hpp>
#include <world/map.hpp>
#include <entities/enemy.hpp>
#include <entities/tower.hpp>
#include <memory>

struct GameConfig {
    int gameWidth = 1280; // Initial window size — user can resize freely
    int gameHeight = 720;
    int fps = 60;
    const char* title = "OpenChaos TD";
};

struct GameData {
    int lives = 20;
    int gold  = 150;
    int score = 0;
    Map map;
    std::vector<Tower> towers;
    std::vector<Enemy> enemies;
};

class Game {
public:
    Game();
    ~Game();

    void Run();

    // Search path of a directory
    std::filesystem::path SearchFolderParentPath(const std::string& folderName, size_t searchDepth);

    // State machine
    void ChangeState(std::unique_ptr<GameState> newState);
    
    const GameConfig& GetGameConfig() const {return m_gameConfig;}
    GameData& GetGameData() {return m_gameData;}

    // Accessors for managers
    AssetManager& GetAssets() {return m_assets;}
    Renderer& GetRenderer() {return m_renderer;}
    InputManager& GetInput() {return m_input;}
    JsonIO& GetJsonIO(){return m_jsonio;}
    PerformanceMonitor& GetMonitor() {return m_monitor;}

private:
    bool m_running = true;

    void LoadAssets();
    void LoadActions();

    // Data and config
    GameConfig m_gameConfig;
    GameData m_gameData;

    // Core managers
    AssetManager m_assets;
    Renderer m_renderer;
    InputManager m_input;
    JsonIO m_jsonio;
    PerformanceMonitor m_monitor;

    // Gamestate management
    std::unique_ptr<GameState> m_currentState;
    std::unique_ptr<GameState> m_pendingState;
    void ApplyPendingState(); // Swaps in m_pendingState at safe point
};