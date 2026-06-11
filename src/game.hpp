#pragma once

#include <raylib.h>
#include <memory>
#include <vector>
#include <states/game_state.hpp>
#include <engine/core/resources.hpp>
#include <engine/features/particle_system.hpp>
#include <engine/features/sound_system.hpp>
#include <engine/core/screen.hpp>
#include <engine/core/input.hpp>
#include <engine/util/file_store.hpp>
#include <game_config.hpp>
#include <engine/util/profiler.hpp>
#include <factory/emitter_presets.hpp>
#include <factory/tower_factory.hpp>
#include <factory/enemy_factory.hpp>
#include <datapack/datapack.hpp>
#include <datapack/datapack_registry.hpp>
#include <world/game_data.hpp>

class Game {
public:
    Game();
    ~Game();

    void Run();

    // Search path of a directory
    std::filesystem::path SearchFolderParentPath(const std::string& folderName, size_t searchDepth);

    // Datapack lifecycle
    // Activating a pack mounts its resources (highest priority) and loads its
    // gameplay data/factories; deactivating frees everything the pack loaded and
    // is safe to call when no pack is active. The selection screen drives this.
    void ActivateDatapack(const Datapack& pack);
    void DeactivateDatapack();
    bool HasActiveDatapack() const { return m_packActive; }
    const std::string& GetActiveDataDir() const { return m_activeDataDir; }
    // Maps live beside the pack's data dir, at "<packRoot>/maps". Derived from the
    // active data dir ("<packRoot>/data") so the map editor needn't track the root.
    std::string GetActiveMapsDir() const;
    DatapackRegistry& GetDatapackRegistry() { return m_registry; }

    // State machine
    void ChangeState(std::unique_ptr<GameState> newState);

    // Overlay stack: push suspends the active state (kept alive, no OnExit) and runs
    // `overlay` on top; pop tears down the top and resumes the suspended state beneath
    // (no re-OnEnter). Lets the in-game pause menu open settings without losing the live
    // match. Applied at the frame boundary like ChangeState.
    void PushState(std::unique_ptr<GameState> overlay);
    void PopState();

    void Quit() { m_running = false; }
    
    const GameConfig& GetGameConfig() const {return m_gameConfig;}
    GameConfig& GetMutableGameConfig() {return m_gameConfig;} // settings menu writes applied values back
    GameData& GetGameData() {return m_gameData;}

    // Accessors for managers
    Resources& GetResources() {return m_resources;}
    Screen& GetScreen() {return m_screen;}
    Input& GetInput() {return m_input;}
    FileStore& GetFileStore(){return m_fileStore;}
    ParticleSystem& GetParticles() {return m_particles;}
    SoundSystem& GetSoundSystem() {return m_soundSystem;}
    EmitterPresets& GetEmitterPresets() {return m_emitterPresets;}
    TowerFactory& GetTowerFactory() {return m_towerFactory;}
    EnemyFactory& GetEnemyFactory() {return m_enemyFactory;}

private:
    bool m_running = true;

    void LoadResources();

    // Active datapack state
    DatapackRegistry m_registry;
    bool m_packActive = false;
    std::string m_activeDataDir; // relative data dir of the active pack, for loaders
    // Asset keys loaded from the active pack, tracked for scoped unload on deactivate.
    std::vector<std::string> m_packTextureKeys;
    std::vector<std::string> m_packSoundKeys;
    std::vector<std::string> m_packMusicKeys;

    // Data and config
    GameConfig m_gameConfig;
    GameData m_gameData;

    // Core managers
    Resources m_resources;
    SoundSystem m_soundSystem;
    ParticleSystem m_particles;
    Screen m_screen;
    Input m_input;
    FileStore m_fileStore;
    Profiler m_profiler;
    EmitterPresets m_emitterPresets;
    TowerFactory m_towerFactory;
    EnemyFactory m_enemyFactory;

    // Gamestate management.
    // Active state is m_currentState (top of the overlay stack); m_suspended holds the
    // states beneath it, deepest first. Depth is normally 1 (2 while settings is open).
    std::unique_ptr<GameState> m_currentState;
    std::vector<std::unique_ptr<GameState>> m_suspended;

    // Deferred transition requested this frame; applied once at the frame boundary.
    enum class PendingOp { None, Replace, Push, Pop };
    PendingOp m_pendingOp = PendingOp::None;
    std::unique_ptr<GameState> m_pendingState; // payload for Replace / Push
    void ApplyPendingState();                  // performs the pending op at a safe point
};