#pragma once

#include <raylib.h>
#include <states/game_state.hpp>
#include <core/asset_manager.hpp>
#include <core/renderer.hpp>
#include <core/input_manager.hpp>
#include <core/jsonio.hpp>
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
    // std::unique_ptr<Map>         map;
    // std::unique_ptr<WaveManager> waveManager;
    // std::unique_ptr<HUD>         hud;
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

    // Accessors for states
    const GameConfig& GetGameConfig() const {return m_gameConfig;}
    AssetManager& GetAssets() {return m_assets;}
    Renderer& GetRenderer() {return m_renderer;}
    InputManager& GetInput() {return m_input;}

    int GetLives() const {return m_data.lives;}
    int GetGold() const {return m_data.gold;}
    int GetScore() const {return m_data.score;}

    void SetLives(int v) {m_data.lives = v;}
    void SetGold(int v) {m_data.gold  = v;}
    void AddScore(int v) {m_data.score += v;}

    // Resets gameplay data
    void Reset();

private:
    void ApplyPendingState(); // Swaps in m_pendingState at safe point
    void LoadAssets();
    void LoadActions();

    GameConfig m_gameConfig;
    GameData m_data;

    AssetManager m_assets;
    Renderer m_renderer;
    JsonIO m_json;
    InputManager m_input;

    // Game status
    bool m_running = true;
    std::unique_ptr<GameState> m_currentState;
    std::unique_ptr<GameState> m_pendingState;
};