#include <states/play_state.hpp>

#include <states/end_state.hpp>
#include <states/menu_state.hpp>
#include <states/settings_state.hpp>
#include <world/map_serialization.hpp>
#include <world/game_paths.hpp>
#include <game.hpp>
#include <raylib.h>
#include <raymath.h>

// --- Base overrides --------------------------------------------------------

void PlayingState::OnEnter(Game& game) {
    game.GetGameData().Reset();
    game.GetParticles().Clear();
    game.GetSoundSystem().PlayMusic("openchaostd_main");

    GameData& gd = game.GetGameData();

    // Resume a save when launched in continue mode; fall back to a fresh map if it fails.
    bool loaded = m_loadFromSave &&
        gd.LoadState(game.GetFileStore(), kSaveGamePath, game.GetTowerFactory());
    if (loaded) {
        // A loaded save embeds its own map; drop any custom selection so a later
        // Restart of this continued game regenerates procedurally.
        gd.m_selectedMapDir.clear();
    } else if (!gd.m_selectedMapDir.empty()) {
        // Build the chosen custom map; fall back to generation if it can't be parsed.
        MapSerialization::MapMeta meta;
        if (!MapSerialization::Load(game.GetFileStore(), gd.m_selectedMapDir, gd.m_map, meta))
            m_mapGenerator.Generate(gd.m_map, 15, 19, 3, 40);
    } else {
        m_mapGenerator.Generate(gd.m_map, 15, 19, 3, 40);
    }

    m_renderSystem.CenterCamera(game.GetGameData().m_map, game.GetScreen());

    float scale   = game.GetGameConfig().hudScale;
    int   screenW = game.GetScreen().GetGameWidth();
    int   screenH = game.GetScreen().GetGameHeight();

    // Static build-bar config (names/textures/costs) captured once from the factory.
    std::vector<TowerBuildOption> towerOptions;
    for (const auto& name : game.GetTowerFactory().GetNames())
        towerOptions.push_back({ name,
                                 game.GetTowerFactory().GetTexture(name),
                                 game.GetTowerFactory().GetCost(name) });

    m_towerHUD.Build(scale, screenW, screenH, towerOptions);
    m_scoreHUD.Build(scale, screenW);
    m_towerInfoHUD.Build(scale);
    m_waveHUD.Build(scale, screenW);
    m_eventLog.Build(scale);

    SoundSystem* ss = &game.GetSoundSystem();
    m_towerHUD.SetSoundSystem(ss);
    m_scoreHUD.SetSoundSystem(ss);
    m_towerInfoHUD.SetSoundSystem(ss);

    m_waveManager.Load(game.GetFileStore(), game.GetEnemyFactory(), game.GetActiveDataDir());

    // A resumed game restarts the wave lookahead from the saved wave number so the next wave
    // (and the HUD preview) match where the player left off, rather than always wave 1.
    if (loaded)
        m_waveManager.PrepareForWave(game.GetGameData().m_waveNumber, game.GetEnemyFactory());

    m_pauseHUD.Build(scale, screenW, screenH);
    m_pauseHUD.SetSoundSystem(ss);
}

void PlayingState::OnExit(Game& game) {
    game.GetSoundSystem().StopMusic();
}

void PlayingState::ProcessInput(Game& game, float dt) {
    // The Cancel/Escape key toggles pause and takes priority; while paused only the
    // overlay receives input so clicks never bleed through to the game grid.
    if (game.GetInput().IsPressed("Cancel")) SetPaused(!m_paused);
    if (m_paused) {
        m_pauseHUD.ProcessInput(game.GetInput());
        HandlePauseSignals(game);
        return;
    }

    if (game.GetInput().IsPressed("Debug")) m_debug = !m_debug;
    if (game.GetInput().IsPressed("Speed")) CycleSpeed();
    if (game.GetInput().IsPressed("WaveInfo")) m_waveHUD.Toggle();
    HandleSaveLoad(game);
    m_renderSystem.ControlCamera(dt, game.GetInput());

    // HUDs consume mouse input first so clicks don't bleed through to the world.
    // Each call is a no-op while that HUD is hidden.
    m_towerHUD.ProcessInput(game.GetInput());
    m_scoreHUD.ProcessInput(game.GetInput(), MakeStatusView(game));
    m_towerInfoHUD.ProcessInput(game.GetInput());
    m_waveHUD.ProcessInput(game.GetInput());
    HandleHudSignals(game);

    Vector2 mouseWorld = game.GetInput().GetWorldMousePosition(m_renderSystem.GetCamera());
    HandleTowerPlacement(game, mouseWorld);

    // Re-point the info panel once this frame's selection changes have settled
    SyncHUDState(game);
}

void PlayingState::Update(Game& game, float dt) {
    if (game.GetGameData().m_victory)
        game.ChangeState(std::make_unique<EndState>(true));

    if (m_gameOver)
        game.ChangeState(std::make_unique<EndState>(false));

    m_eventLog.Update(dt); // HUD fade tracks real time, not game speed

    // Run the simulation once per speed step. Each sub-step uses the real frame dt so per-frame
    // timing (tower cooldowns, spawn pacing) stays accurate instead of feeding one oversized step.
    for (int s = 0; s < kSpeedSteps[m_speedIndex] && !m_paused && !m_gameOver && !game.GetGameData().m_victory; s++)
        StepSimulation(game, dt);
}

void PlayingState::StepSimulation(Game& game, float dt) {
    GameData& data = game.GetGameData();
    m_waveManager.Update(dt, data, m_worldSystem, game.GetEnemyFactory());

    m_enemySystem.TickEnemies(dt, data.m_enemies, data.m_map, game.GetParticles());
    m_enemySystem.FollowPath(dt, data.m_enemies, data.m_map);

    m_towerSystem.Update(dt, data.m_towers, data.m_enemies, data.m_attacks, game.GetParticles(), game.GetSoundSystem());
    m_towerSystem.TickAttacks(dt, data.m_enemies, data.m_attacks, game.GetParticles());
    game.GetParticles().Tick(dt);

    m_worldSystem.CheckEnemyReachedCore(data);
    m_worldSystem.CheckEnemyDead(data, game.GetEnemyFactory(), game.GetParticles(), game.GetSoundSystem());
    m_worldSystem.CheckGameOver(m_gameOver, data);
}

void PlayingState::Draw(Game& game) {
    ClearBackground(DARKGRAY);
    Vector2 mouseWorld = game.GetInput().GetWorldMousePosition(m_renderSystem.GetCamera());

    BeginMode2D(m_renderSystem.GetCamera());
    m_renderSystem.DrawMap(game.GetGameData().m_map, game.GetResources());
    m_renderSystem.DrawPaths(game.GetGameData().m_map);
    if (m_debug) {
        m_renderSystem.DebugDrawMap(game.GetGameData().m_map);
        m_renderSystem.DebugDrawEnemies(game.GetGameData().m_enemies);
    }
    m_renderSystem.DrawTowers(game.GetGameData().m_towers, game.GetResources());
    m_renderSystem.DrawRangeIndicator(m_selection.towerKey, game.GetGameData().m_map, game.GetGameData().m_towers, mouseWorld);
    if (!game.GetGameData().m_waveActive &&
        m_selection.towerKey == DenseSlotMap<Tower>::INVALID_KEY &&
        !m_towerHUD.GetSelectedTower().empty()) {
        int x, y;
        if (game.GetGameData().m_map.WorldToTile(mouseWorld, x, y)) {
            const std::string& name = m_towerHUD.GetSelectedTower();
            float half = game.GetGameData().m_map.GetTileSize() / 2.0f;
            Vector2 tileCenter = Vector2Add(game.GetGameData().m_map.TileToWorld(x, y), {half, half});
            Texture2D& tex = game.GetResources().GetTexture(game.GetTowerFactory().GetTexture(name));
            m_renderSystem.DrawGhostTower(tileCenter, game.GetTowerFactory().GetRange(name), tex);
        }
    }
    m_renderSystem.DrawEnemies(game.GetGameData().m_enemies, game.GetResources());
    m_renderSystem.DrawAttacks(game.GetGameData().m_attacks);
    game.GetParticles().Draw();
    EndMode2D();

    // Draw order: info panel last so it sits on top. Hidden HUDs skip themselves.
    m_towerHUD.Draw(MakeBuildBarView(game), game.GetResources());
    m_scoreHUD.Draw(MakeStatusView(game));
    m_waveHUD.Draw(MakeWaveView(), game.GetResources());
    m_eventLog.Draw();
    m_towerInfoHUD.Draw();
    m_pauseHUD.Draw(); // last so the overlay dims everything; no-op while not paused
}

// --- Input helpers ---------------------------------------------------------

void PlayingState::HandleHudSignals(Game& game) {
    if (m_towerInfoHUD.WasSellRequested()) {
        if (Tower* tower = game.GetGameData().m_towers.Get(m_selection.towerKey)) {
            int x, y;
            if (game.GetGameData().m_map.WorldToTile(tower->m_position, x, y)) {
                game.GetGameData().m_gold += static_cast<int>(tower->m_cost * game.GetGameData().m_sellRefundRate);
                m_worldSystem.RemoveTower(x, y, game.GetGameData());
            }
        }
        m_selection.towerKey = DenseSlotMap<Tower>::INVALID_KEY;
    }

    if (m_towerInfoHUD.WasTargetingCycleRequested())
        if (Tower* tower = game.GetGameData().m_towers.Get(m_selection.towerKey))
            if (AttackModule* attack = tower->GetAttack())
                attack->m_targetingMode = NextTargetingMode(attack->m_targetingMode);

    if (m_towerInfoHUD.WasUpgradeRequested())
        UpgradeSelectedTower(game);

    if (m_scoreHUD.WasWaveRequested())
        m_waveManager.StartWave(game.GetGameData(), game.GetEnemyFactory());

    if (m_scoreHUD.WasAutoToggled())
        m_waveManager.ToggleAutoSpawn();

    if (m_scoreHUD.WasSpeedToggled())
        CycleSpeed();

    if (m_scoreHUD.WasWaveInfoToggled())
        m_waveHUD.Toggle();
}

void PlayingState::UpgradeSelectedTower(Game& game) {
    Tower* tower = game.GetGameData().m_towers.Get(m_selection.towerKey);
    if (!tower || !tower->m_upgrades) return;

    const auto& ups = *tower->m_upgrades;
    if (tower->m_level >= static_cast<int>(ups.size())) return; // already max level

    const TowerUpgrade& up = ups[tower->m_level];
    if (game.GetGameData().m_gold < up.m_cost) return;

    game.GetGameData().m_gold -= up.m_cost;
    game.GetTowerFactory().ApplyUpgradeStats(*tower, up);

    tower->m_cost += up.m_cost; // sell refund reflects total invested
    tower->m_level++;
}

void PlayingState::SaveGame(Game& game) {
    game.GetGameData().SaveState(game.GetFileStore(), kSaveGamePath, game.GetActiveDataDir());
    m_eventLog.Add("Game saved");
}

bool PlayingState::LoadGame(Game& game) {
    if (!game.GetGameData().LoadState(game.GetFileStore(), kSaveGamePath, game.GetTowerFactory())) {
        m_eventLog.Add("No valid save to load");
        return false;
    }

    // Restored tower keys are stable, but drop the inspection selection defensively, recentre on
    // the installed map, and re-prime the wave lookahead to the restored wave number.
    m_selection.towerKey = DenseSlotMap<Tower>::INVALID_KEY;
    m_renderSystem.CenterCamera(game.GetGameData().m_map, game.GetScreen());
    m_waveManager.PrepareForWave(game.GetGameData().m_waveNumber, game.GetEnemyFactory());
    m_eventLog.Add("Game loaded");
    return true;
}

void PlayingState::HandleSaveLoad(Game& game) {
    // The F-key shortcuts are only offered between waves (no live enemies or attacks to persist).
    if (game.GetGameData().m_waveActive) return;

    if (game.GetInput().IsPressed("SaveGame")) SaveGame(game);
    if (game.GetInput().IsPressed("LoadGame")) LoadGame(game);
}

void PlayingState::HandleTowerPlacement(Game& game, Vector2 mouseWorld) {
    if (!game.GetInput().IsMousePressed(MOUSE_LEFT_BUTTON)) return;
    if (game.GetInput().IsMouseInputConsumed()) return;

    int x, y;
    if (!game.GetGameData().m_map.WorldToTile(mouseWorld, x, y)) {
        // Clicked outside the map — deselect
        m_selection.towerKey = DenseSlotMap<Tower>::INVALID_KEY;
        return;
    }

    Tile& tile = game.GetGameData().m_map.Get(x, y);

    // Tile has a tower — select it regardless of wave state
    if (tile.m_towerKey != DenseSlotMap<Tower>::INVALID_KEY) {
        m_selection.towerKey = tile.m_towerKey;
        return;
    }

    // Empty tile — always deselect the inspected tower
    m_selection.towerKey = DenseSlotMap<Tower>::INVALID_KEY;

    // Placement is blocked during waves or when no tower type is selected
    if (game.GetGameData().m_waveActive) {
        if (!m_towerHUD.GetSelectedTower().empty())
            m_eventLog.Add("Cannot place towers during a wave!");
        return;
    }
    if (m_towerHUD.GetSelectedTower().empty()) return;
    int cost = game.GetTowerFactory().GetCost(m_towerHUD.GetSelectedTower());
    if (game.GetGameData().m_gold >= cost) {
        auto built = game.GetTowerFactory().Create(m_towerHUD.GetSelectedTower());
        if (!built) return;
        if (m_worldSystem.PlaceTower(x, y, *built, game.GetGameData())) {
            game.GetGameData().m_gold -= cost;
            m_towerHUD.ClearSelection();
        }
    }
}

void PlayingState::SyncHUDState(Game& game) {
    if (m_selection.towerKey != DenseSlotMap<Tower>::INVALID_KEY) {
        if (Tower* tower = game.GetGameData().m_towers.Get(m_selection.towerKey)) {
            Vector2 screenPos = GetWorldToScreen2D(tower->m_position, m_renderSystem.GetCamera());
            m_towerInfoHUD.SetTarget(MakeTowerInfoView(game, *tower, screenPos, true));
        } else {
            m_towerInfoHUD.Hide();
        }
        m_selection.hoveredTowerName.clear();
        return;
    }

    // Nothing selected — show a preview panel for the hovered tower-bar button
    Vector2 mousePos = game.GetInput().GetMousePosition();
    const std::string& hovered = m_towerHUD.GetHoveredTower(mousePos);
    if (hovered.empty()) {
        m_selection.hoveredTowerName.clear();
        m_towerInfoHUD.Hide();
        return;
    }

    // Only rebuild the preview tower when the hovered type changes
    if (m_selection.hoveredTowerName != hovered) {
        m_selection.hoveredTowerName = hovered;
        if (auto built = game.GetTowerFactory().Create(hovered))
            m_hoveredTowerCache = std::move(*built);
    }
    Vector2 topCenter = m_towerHUD.GetHoveredButtonTopCenter(mousePos);
    m_towerInfoHUD.SetTarget(MakeTowerInfoView(game, m_hoveredTowerCache, topCenter, false));
}

// --- Pause menu ------------------------------------------------------------

void PlayingState::SetPaused(bool paused) {
    m_paused = paused;
    if (paused) m_pauseHUD.Show();
    else m_pauseHUD.Hide();
}

void PlayingState::HandlePauseSignals(Game& game) {
    if (m_pauseHUD.WasResumeRequested())
        SetPaused(false);

    // Open settings as an overlay; the match stays paused and alive beneath it, so
    // popping back lands on this same pause menu with all state intact.
    if (m_pauseHUD.WasSettingsRequested())
        game.PushState(std::make_unique<SettingsState>());

    if (m_pauseHUD.WasSaveRequested())
        SaveGame(game);

    // A successful load installs a fresh between-waves state, so leave the pause overlay.
    if (m_pauseHUD.WasLoadRequested() && LoadGame(game))
        SetPaused(false);

    if (m_pauseHUD.WasRestartRequested())
        game.ChangeState(std::make_unique<PlayingState>());

    if (m_pauseHUD.WasMainMenuRequested())
        game.ChangeState(std::make_unique<MenuState>());
}

// --- View builders ---------------------------------------------------------
// Snapshot gameplay state into read-only views so the HUDs never touch GameData/WaveManager.

StatusView PlayingState::MakeStatusView(Game& game) {
    const GameData& data = game.GetGameData();
    StatusView view;
    view.m_lives       = data.m_lives;
    view.m_gold        = data.m_gold;
    view.m_waveNumber  = data.m_waveNumber;
    view.m_victoryWave = m_waveManager.GetVictoryWave();
    view.m_waveActive  = data.m_waveActive;
    view.m_autoSpawn   = m_waveManager.IsAutoSpawn();
    view.m_speed       = kSpeedSteps[m_speedIndex];
    return view;
}

BuildBarView PlayingState::MakeBuildBarView(Game& game) {
    BuildBarView view;
    view.m_gold = game.GetGameData().m_gold;
    return view;
}

WaveView PlayingState::MakeWaveView() {
    WaveView view;
    view.m_budget = m_waveManager.GetNextWaveBudget();

    const auto& groups = m_waveManager.GetNextWaveDef().m_groups;
    const auto& prototypes = m_waveManager.GetPreviewPrototypes();
    view.m_entries.reserve(groups.size());
    for (const auto& g : groups) {
        WaveEnemyEntry entry;
        entry.m_count = g.m_count;
        entry.m_name = g.m_enemyType;
        auto it = prototypes.find(g.m_enemyType);
        if (it != prototypes.end()) {
            const Enemy& proto = it->second;
            entry.m_hasProto = true;
            entry.m_level = proto.m_level;
            entry.m_textureKey = proto.m_presentation.m_texture;
            // Every module appends its rows (Health/Speed, Armor, Regen, Shield, Split, Immune, ...).
            for (const auto& mod : proto.m_modules)
                mod->DescribeStats(entry.m_stats);
        }
        view.m_entries.push_back(std::move(entry));
    }
    return view;
}

TowerInfoView PlayingState::MakeTowerInfoView(Game& game, const Tower& tower, Vector2 screenPos, bool interactive) {
    const GameData& data = game.GetGameData();
    TowerInfoView view;
    view.m_name = tower.m_name;
    view.m_description = tower.m_description;
    // Every module appends its rows: AttackModule core stats, then each effect module's lines.
    for (const auto& mod : tower.m_modules)
        mod->DescribeStats(view.m_statLines);

    const AttackModule* attack = tower.GetAttack(); // null = a wall (no combat UI)
    view.m_hasAttack    = attack != nullptr;
    view.m_interactive  = interactive;
    view.m_waveActive   = data.m_waveActive;
    view.m_sellRefund   = static_cast<int>(tower.m_cost * data.m_sellRefundRate);
    view.m_level        = tower.m_level;
    view.m_upgradeCount = tower.m_upgrades ? static_cast<int>(tower.m_upgrades->size()) : 0;

    if (view.m_upgradeCount > 0) {
        if (tower.m_level >= view.m_upgradeCount) {
            view.m_upgradeAtMax = true;
        } else {
            const TowerUpgrade& up = (*tower.m_upgrades)[tower.m_level];
            view.m_upgradeCost = up.m_cost;
            view.m_upgradeReady = data.m_gold >= up.m_cost;
            up.Describe(view.m_upgradePreview);
        }
    }

    if (attack)
        view.m_targetingName = TargetingModeName(attack->m_targetingMode);

    view.m_screenPos = screenPos;
    view.m_screenW = game.GetScreen().GetGameWidth();
    view.m_screenH = game.GetScreen().GetGameHeight();
    return view;
}
