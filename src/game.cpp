#include <game.hpp>
#include <engine/core/text.hpp>
#include <raylib.h>
#include <cassert>
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <filesystem>
#include <states/menu_state.hpp>

namespace {

// Shrinks and recenters the window if its physical footprint would exceed the
// monitor. Under HiDPI the requested logical size can map to a larger framebuffer
// (1200pt is ~1800px at 1.5x), so a 1200x1200 request can overflow the screen.
// We compare in framebuffer pixels (GetRenderWidth, same space as the monitor
// size) and scale the logical window size by the resulting fit factor.
void ClampWindowToMonitor() {
    const int monitor = GetCurrentMonitor();

    // Budget in physical pixels, leaving a margin for window-manager chrome.
    const float margin = 0.92f;
    const float maxPxW = GetMonitorWidth(monitor)  * margin;
    const float maxPxH = GetMonitorHeight(monitor) * margin;

    // Current window footprint in physical pixels.
    const float curPxW = static_cast<float>(GetRenderWidth());
    const float curPxH = static_cast<float>(GetRenderHeight());

    const float fit = std::min({1.0f, maxPxW / curPxW, maxPxH / curPxH});
    if (fit >= 1.0f)
        return; // already fits

    // SetWindowSize/SetWindowPosition operate in logical points; scale the
    // current logical size by the same factor (uniform => aspect preserved).
    const int newLogicalW = static_cast<int>(GetScreenWidth()  * fit);
    const int newLogicalH = static_cast<int>(GetScreenHeight() * fit);
    SetWindowSize(newLogicalW, newLogicalH);

    // Recenter: convert the monitor's pixel size to logical points via the
    // measured render/screen ratio (== 1.0 when the framebuffer wasn't scaled).
    const float pxPerPoint = (GetScreenWidth() > 0)
        ? static_cast<float>(GetRenderWidth()) / static_cast<float>(GetScreenWidth())
        : 1.0f;
    const Vector2 monPos = GetMonitorPosition(monitor);
    const float monLogicalW = GetMonitorWidth(monitor)  / pxPerPoint;
    const float monLogicalH = GetMonitorHeight(monitor) / pxPerPoint;
    SetWindowPosition(static_cast<int>(monPos.x + (monLogicalW - newLogicalW) * 0.5f),
                      static_cast<int>(monPos.y + (monLogicalH - newLogicalH) * 0.5f));
    TraceLog(LOG_INFO, "DISPLAY: clamped window to %dx%d logical (fit=%.2f)", newLogicalW, newLogicalH, fit);
}

} // namespace

// Constructor / Destructor
Game::Game() {
    m_fileStore.SetRootPath(SearchFolderParentPath("resources", 5).parent_path());
    m_gameConfig.Load(m_fileStore);

    // Deliberately NOT FLAG_WINDOW_HIGHDPI: Screen owns all virtual->window
    // scaling, but raylib's HIGHDPI path bakes a screenScale into the MODELVIEW
    // that BeginMode2D/EndMode2D re-apply, stacking a stray DPI factor on the HUD
    // and world. Without the flag screenScale stays identity and our projection is
    // the single source of truth; text still rasterizes at native resolution.
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(m_gameConfig.gameWidth, m_gameConfig.gameHeight, m_gameConfig.title.c_str());
    ClampWindowToMonitor();
    m_gameConfig.ApplyIcon();
    SetTargetFPS(m_gameConfig.fps);
    SetExitKey(KEY_NULL);
    InitAudioDevice();

    // gameWidth/gameHeight is the fixed virtual/design resolution; the actual
    // window may have been clamped above, and Screen letterboxes between them.
    m_screen.Init(m_gameConfig.gameWidth, m_gameConfig.gameHeight);
    Text::Init(m_fileStore);

    // Gameplay data and assets are no longer loaded here — they belong to a
    // datapack and load only once the player selects one (see ActivateDatapack).
    LoadResources();
    m_soundSystem.SetMusicVolume(m_gameConfig.musicVolume);
    m_soundSystem.SetSfxVolume(m_gameConfig.sfxVolume);
    m_input.Load(m_fileStore);

    // Discover installed datapacks for the selection screen.
    m_registry.Scan(m_fileStore);

    // Init initial state
    m_currentState = std::make_unique<MenuState>();
    m_currentState->OnEnter(*this);
}

Game::~Game() {
    if (m_currentState)
        m_currentState->OnExit(*this);
    for (auto it = m_suspended.rbegin(); it != m_suspended.rend(); ++it)
        (*it)->OnExit(*this);

    Text::Shutdown();
    CloseAudioDevice();
    CloseWindow();
    m_profiler.Print();
}

// Run
void Game::Run() {
    while (!WindowShouldClose() && m_running) {
        const float dt = GetFrameTime();

        m_screen.OnResize();

        // Update
        m_profiler.Begin("Update");
            m_soundSystem.Tick(dt);
            m_input.Update(m_screen);
            m_currentState->ProcessInput(*this, dt);
            m_currentState->Update(*this, dt);
        m_profiler.End("Update");

        // Draw
        m_profiler.Begin("Draw");
            m_screen.BeginFrame();
                m_currentState->Draw(*this);
            m_screen.EndFrame();
        m_profiler.End("Draw");

        // Swap State
        ApplyPendingState();
    }
}

std::filesystem::path Game::SearchFolderParentPath(const std::string& folderName, size_t searchDepth){
    std::filesystem::path currentDir = GetApplicationDirectory();
    std::cout << "Searching for '" << folderName << "' folder starting from " << currentDir << "\n";

    for (size_t i = 0; i < searchDepth; i++) {
        std::filesystem::path potentialPath = currentDir / folderName;

        if (std::filesystem::exists(potentialPath) && std::filesystem::is_directory(potentialPath)) {
            std::cout << "Found asset directory: " << potentialPath << "\n";
            return potentialPath;
        }

        currentDir = currentDir.parent_path();
    }

    throw std::runtime_error("Resources: could not find directory '" + folderName + "' within " + std::to_string(searchDepth) + " levels up");
}

// Asset loading
void Game::LoadResources() {
    // The global "resources" folder is the lowest-priority (fallback) search root
    // and holds only engine/menu-level assets. Gameplay assets live in datapacks
    // and are mounted on top of this root when a pack is activated.
    m_resources.SetAssetPath(SearchFolderParentPath("resources", 5));
    m_resources.LoadSoundsFromDir("sounds");
    m_soundSystem.Init(m_resources);
}

// Datapack lifecycle
void Game::ActivateDatapack(const Datapack& pack) {
    // Switching packs: tear the current one down first so nothing leaks or stacks.
    if (m_packActive)
        DeactivateDatapack();

    m_activeDataDir = pack.DataDir();

    // Mount the pack's resources as the highest-priority search root so its assets
    // shadow any global asset of the same key, then load them (tracking keys for unload).
    std::string resourcesRoot =
        (std::filesystem::path(m_fileStore.GetRootPath()) / pack.ResourcesDir()).string();
    m_resources.PushSearchPath(resourcesRoot);
    m_packTextureKeys = m_resources.LoadTexturesFromDir("textures");
    m_packSoundKeys   = m_resources.LoadSoundsFromDir("sounds");
    m_packMusicKeys   = m_resources.LoadMusicFromDir("music");

    // Load gameplay data from the pack. Presets first: the factories resolve
    // emitter preset names while building their templates.
    m_emitterPresets.Load(m_fileStore, m_activeDataDir);
    m_gameData.Load(m_fileStore, m_activeDataDir);
    m_towerFactory.Load(m_fileStore, m_emitterPresets, m_activeDataDir);
    m_enemyFactory.Load(m_fileStore, m_emitterPresets, m_activeDataDir);

    m_packActive = true;
}

void Game::DeactivateDatapack() {
    if (!m_packActive) return;

    // Stop any pack music before freeing the streams it points into.
    m_soundSystem.StopMusic();

    for (const auto& key : m_packTextureKeys) m_resources.UnloadTextureKey(key);
    for (const auto& key : m_packSoundKeys)   m_resources.UnloadSoundKey(key);
    for (const auto& key : m_packMusicKeys)   m_resources.UnloadMusicKey(key);
    m_packTextureKeys.clear();
    m_packSoundKeys.clear();
    m_packMusicKeys.clear();

    // Unmount the pack's resource root (the global base root stays).
    m_resources.PopSearchPath();

    // Drop loaded templates/presets.
    m_towerFactory.Clear();
    m_enemyFactory.Clear();
    m_emitterPresets.Clear();

    m_activeDataDir.clear();
    m_packActive = false;
}

std::string Game::GetActiveMapsDir() const {
    // m_activeDataDir is "<packRoot>/data"; maps sit beside it at "<packRoot>/maps".
    const std::string suffix = "/data";
    std::string root = m_activeDataDir;
    if (root.size() >= suffix.size() &&
        root.compare(root.size() - suffix.size(), suffix.size(), suffix) == 0)
        root.erase(root.size() - suffix.size());
    return root + "/maps";
}

// State machine — all transitions are deferred and applied at the frame boundary.
void Game::ChangeState(std::unique_ptr<GameState> newState) {
    m_pendingState = std::move(newState);
    m_pendingOp = PendingOp::Replace;
}

void Game::PushState(std::unique_ptr<GameState> overlay) {
    m_pendingState = std::move(overlay);
    m_pendingOp = PendingOp::Push;
}

void Game::PopState() {
    m_pendingOp = PendingOp::Pop;
}

// Helpers
void Game::ApplyPendingState() {
    switch (m_pendingOp) {
        case PendingOp::None:
            return;
        case PendingOp::Replace:
            if (m_currentState)
                m_currentState->OnExit(*this);
            m_currentState = std::move(m_pendingState);
            m_currentState->OnEnter(*this);
            break;
        case PendingOp::Push:
            // Suspend the active state (no OnExit so it keeps its live data and music).
            if (m_currentState)
                m_suspended.push_back(std::move(m_currentState));
            m_currentState = std::move(m_pendingState);
            m_currentState->OnEnter(*this);
            break;
        case PendingOp::Pop:
            // Tear down the overlay and resume the state beneath (no re-OnEnter).
            assert(!m_suspended.empty() && "PopState with nothing suspended");
            if (m_suspended.empty())
                break; // defensive: keep the current state rather than going stateless
            if (m_currentState)
                m_currentState->OnExit(*this);
            m_currentState = std::move(m_suspended.back());
            m_suspended.pop_back();
            break;
    }
    m_pendingOp = PendingOp::None;
    m_pendingState.reset();
}