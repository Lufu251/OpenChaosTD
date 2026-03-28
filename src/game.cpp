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
    SetExitKey(KEY_NULL);
    InitAudioDevice();

    m_renderer.Init(m_gameConfig.gameWidth, m_gameConfig.gameHeight);

    m_jsonio.SetRootPath(SearchFolderParentPath("assets", 5).parent_path());

    LoadAssets();
    LoadActions();

    // Init initial state
    m_currentState = std::make_unique<MenuState>();
    m_currentState->OnEnter(*this);
}

Game::~Game() {
    if (m_currentState)
        m_currentState->OnExit(*this);

    CloseAudioDevice();
    CloseWindow();
    m_monitor.Print();
}

// Run
void Game::Run() {
    while (!WindowShouldClose() && m_running) {
        const float dt = GetFrameTime();

        // Update
        m_monitor.Begin("Update");
            m_input.Update(m_renderer);
            m_currentState->ProcessInput(*this);
            m_currentState->Update(*this, dt);
        m_monitor.End("Update");

        // Draw
        m_monitor.Begin("Draw");
            m_renderer.BeginFrame();
                m_currentState->Draw(*this);
            m_renderer.EndFrame();
        m_monitor.End("Draw");

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

    // Tower textures
    m_assets.LoadTexture("tower_zapper", "textures/tower_zapper.png");
    m_assets.LoadTexture("tower_freezer", "textures/tower_freezer.png");
    m_assets.LoadTexture("tower_sniper", "textures/tower_sniper.png");
    m_assets.LoadTexture("tower_wall", "textures/tower_wall.png");

    // Tile textures
    m_assets.LoadTexture("tile_grass", "textures/tile_grass.png");
    m_assets.LoadTexture("tile_nest", "textures/tile_nest.png");
    m_assets.LoadTexture("tile_core", "textures/tile_core.png");
    m_assets.LoadTexture("tile_rock", "textures/tile_rock.png");

    // Enemy textures
    
}

// Action loading
void Game::LoadActions() {
    m_input.AddAction("Up", KEY_W);
    m_input.AddAction("Down", KEY_S);
    m_input.AddAction("Right", KEY_D);
    m_input.AddAction("Left", KEY_A);

    m_input.AddAction("Confirm", KEY_ENTER);
    m_input.AddAction("Cancle", KEY_ESCAPE);
    m_input.AddAction("Debug", KEY_GRAVE);
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