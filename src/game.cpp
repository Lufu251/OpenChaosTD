#include <game.hpp>
#include <raylib.h>
#include <states/menu_state.hpp>
#include <iostream>
#include <stdexcept>
#include <filesystem>
#include <nlohmann/json.hpp>

// Constructor / Destructor
Game::Game() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE); // Allow free window resizing
    SetConfigFlags(FLAG_WINDOW_HIGHDPI); // Enable High DPI scaling

    InitWindow(m_gameConfig.gameWidth, m_gameConfig.gameHeight, m_gameConfig.title);
    SetTargetFPS(m_gameConfig.fps);
    InitAudioDevice();

    m_renderer.Init(m_gameConfig.gameWidth, m_gameConfig.gameHeight);

    LoadAssets();
    LoadActions();

    m_json.SetRootPath(SearchFolderParentPath("assets", 5).parent_path());

    nlohmann::json j;
    j["pi"] = 3.141;
    j["happy"] = true;
    j["name"] = "Niels";

    m_json.Save("saves/slot1", j);
    m_json.Delete("saves/slot1");

    // Start with menu state
    m_currentState = std::make_unique<MenuState>();
    m_currentState->OnEnter(*this);
}

Game::~Game() {
    if (m_currentState)
        m_currentState->OnExit(*this);

    CloseAudioDevice();
    CloseWindow();
    monitor.Print();
}

// Run
void Game::Run() {
    while (!WindowShouldClose() && m_running) {
        const float dt = GetFrameTime();

        // Update
        monitor.Begin("Update");
            m_input.Update(m_renderer);
            m_currentState->ProcessInput(*this);
            m_currentState->Update(*this, dt);
        monitor.End("Update");

        // Draw
        monitor.Begin("Draw");
            m_renderer.BeginFrame();
                m_currentState->Draw(*this);
            m_renderer.EndFrame();
        monitor.End("Draw");

        // Swap State
        ApplyPendingState();
    }
}

std::filesystem::path Game::SearchFolderParentPath(const std::string& folderName, size_t searchDepth){
    std::filesystem::path currentDir = GetWorkingDirectory();
    std::cout << "Searching for '" << folderName << "' folder starting from " << currentDir << "\n";

    for (size_t i = 0; i < searchDepth; i++) {
        std::filesystem::path potentialPath = currentDir / folderName;

        if (std::filesystem::exists(potentialPath) && std::filesystem::is_directory(potentialPath)) {
            std::cout << "Found asset directory: " << potentialPath << "\n";
            return potentialPath;
        }

        currentDir = currentDir.parent_path();
    }

    throw std::runtime_error("AssetManager: could not find directory '" + folderName + "' within " + std::to_string(searchDepth) + " levels up");
    return "NotFound";
}

// Asset loading
void Game::LoadAssets() {
    // Walk up the directory tree to find the "assets" folder
    m_assets.SetAssetPath(SearchFolderParentPath("assets", 5));

    // Textures
    m_assets.LoadTexture("tower_zapper", "textures/tower_zapper.png");
    m_assets.LoadTexture("tower_freezer", "textures/tower_freezer.png");
    m_assets.LoadTexture("tower_sniper", "textures/tower_sniper.png");
    m_assets.LoadTexture("tower_wall", "textures/tower_wall.png");
}

// Action loading
void Game::LoadActions() {
    m_input.AddAction("Confirm", KEY_ENTER);
    m_input.AddAction("Cancle", KEY_ESCAPE);
}

// State machine
void Game::ChangeState(std::unique_ptr<GameState> newState) {
    // Store for deferred application — never swap mid-frame
    m_pendingState = std::move(newState);
}

// Helpers
void Game::ApplyPendingState() {
    if (!m_pendingState) return;

    if (m_currentState)
        m_currentState->OnExit(*this);

    m_currentState = std::move(m_pendingState);
    m_currentState->OnEnter(*this);
}

void Game::Reset() {
    m_data.lives = 20;
    m_data.gold  = 150;
    m_data.score = 0;
}