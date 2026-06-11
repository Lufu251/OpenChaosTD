#include <states/settings_state.hpp>
#include <engine/core/text.hpp>
#include <engine/core/draw_helpers.hpp>
#include <game.hpp>
#include <engine/core/input.hpp>
#include <raylib.h>
#include <nlohmann/json.hpp>
#include <cmath>
#include <memory>

namespace {
    // Warning tint reused for duplicate-key highlighting and the conflict banner.
    constexpr Color kWarnColor = {255, 180, 0, 255};
}

// --- ConfigValues comparison -----------------------------------------------

bool SettingsState::ConfigValues::operator==(const ConfigValues& o) const {
    if (musicVolume != o.musicVolume || sfxVolume != o.sfxVolume ||
        fps != o.fps || hudScale != o.hudScale)
        return false;
    if (bindings.size() != o.bindings.size()) return false;
    for (size_t g = 0; g < bindings.size(); g++) {
        if (bindings[g].category != o.bindings[g].category) return false;
        if (bindings[g].bindings.size() != o.bindings[g].bindings.size()) return false;
        for (size_t b = 0; b < bindings[g].bindings.size(); b++) {
            if (bindings[g].bindings[b].action != o.bindings[g].bindings[b].action) return false;
            if (bindings[g].bindings[b].key != o.bindings[g].bindings[b].key) return false;
        }
    }
    return true;
}

// --- Defaults / loading ----------------------------------------------------

std::vector<SettingsState::BindingGroup> SettingsState::DefaultBindings() {
    return {
        {"Movement",  {{"Up", "W"}, {"Down", "S"}, {"Left", "A"}, {"Right", "D"}}},
        {"Interface", {{"Confirm", "ENTER"}, {"Cancel", "ESCAPE"}, {"WaveInfo", "TAB"}}},
        {"Game",      {{"Speed", "SPACE"}, {"Debug", "GRAVE"}}},
    };
}

std::vector<SettingsState::BindingGroup> SettingsState::LoadBindings(Game& game) {
    // Start from the canonical layout (stable, nicely ordered) and overlay any keys
    // found in the file onto the matching actions. Faithful to disk for the fixed
    // action set while keeping a predictable UI order regardless of file key order.
    auto bindings = DefaultBindings();
    if (!game.GetFileStore().Exists("config/keybindings.json"))
        return bindings;

    auto j = game.GetFileStore().LoadJson("config/keybindings.json");
    for (auto& [category, actions] : j.items()) {
        if (!actions.is_object()) continue;
        for (auto& [action, value] : actions.items()) {
            if (!value.is_string()) continue;
            std::string key = value.get<std::string>();
            for (auto& grp : bindings)
                for (auto& b : grp.bindings)
                    if (b.action == action) b.key = key;
        }
    }
    return bindings;
}

// --- Lifecycle -------------------------------------------------------------

void SettingsState::OnEnter(Game& game) {
    const GameConfig& cfg = game.GetGameConfig();
    m_snapshot.musicVolume = cfg.musicVolume;
    m_snapshot.sfxVolume = cfg.sfxVolume;
    m_snapshot.fps = cfg.fps;
    m_snapshot.hudScale = cfg.hudScale;
    m_snapshot.bindings = LoadBindings(game);

    m_working = m_snapshot;

    Layout(game);
    SyncWidgetsFromWorking();
}

void SettingsState::OnExit(Game& /*game*/) {}

void SettingsState::Layout(Game& game) {
    float gw = static_cast<float>(game.GetScreen().GetGameWidth());
    float gh = static_cast<float>(game.GetScreen().GetGameHeight());

    // Audio + HUD-scale sliders
    m_musicSlider.m_rect = {kSliderX, kMusicY, kSliderW, kSliderH};
    m_musicSlider.m_min = 0.0f; m_musicSlider.m_max = 1.0f; m_musicSlider.m_step = 0.05f;
    m_sfxSlider.m_rect = {kSliderX, kSfxY, kSliderW, kSliderH};
    m_sfxSlider.m_min = 0.0f; m_sfxSlider.m_max = 1.0f; m_sfxSlider.m_step = 0.05f;
    m_hudScaleSlider.m_rect = {kSliderX, kHudScaleY, kSliderW, kSliderH};
    m_hudScaleSlider.m_min = 1.0f; m_hudScaleSlider.m_max = 2.0f; m_hudScaleSlider.m_step = 0.25f;

    // FPS stepper: [<] value [>]
    m_fpsDownBtn.m_label = "<";
    m_fpsDownBtn.m_rect = {kSliderX, kFpsY, 40.0f, 30.0f};
    m_fpsValueRect = {kSliderX + 48.0f, kFpsY, 120.0f, 30.0f};
    m_fpsUpBtn.m_label = ">";
    m_fpsUpBtn.m_rect = {kSliderX + 176.0f, kFpsY, 40.0f, 30.0f};

    // Keybinding rows grouped by category
    m_groupHeaders.clear();
    m_keyRows.clear();
    float y = kCtrlTopY;
    for (auto& grp : m_working.bindings) {
        m_groupHeaders.push_back({grp.category, y});
        y += kGroupHeaderH;
        for (auto& b : grp.bindings) {
            KeyRow row;
            row.category = grp.category;
            row.action = b.action;
            row.cell.m_rect = {kKeyCellX, y, kKeyCellW, kKeyCellH};
            m_keyRows.push_back(row);
            y += kRowH;
        }
        y += kGroupGap;
    }
    m_controlsBottomY = y;

    // Bottom action buttons, centered as a row
    const float bw = 200.0f, bh = 50.0f, gap = 24.0f;
    float total = bw * 4.0f + gap * 3.0f;
    float sx = (gw - total) / 2.0f;
    m_saveBtn.m_label = "SAVE";
    m_saveBtn.m_rect = {sx, kBottomBtnY, bw, bh};
    m_discardBtn.m_label = "DISCARD";
    m_discardBtn.m_rect = {sx + (bw + gap), kBottomBtnY, bw, bh};
    m_resetBtn.m_label = "RESET DEFAULTS";
    m_resetBtn.m_rect = {sx + 2.0f * (bw + gap), kBottomBtnY, bw, bh};
    m_backBtn.m_label = "BACK";
    m_backBtn.m_rect = {sx + 3.0f * (bw + gap), kBottomBtnY, bw, bh};

    // Unsaved-changes dialog
    const float dw = 540.0f, dh = 240.0f;
    m_dialogPanel = {(gw - dw) / 2.0f, (gh - dh) / 2.0f, dw, dh};
    const float dbw = 160.0f, dbh = 46.0f, dgap = 16.0f;
    float dtotal = dbw * 3.0f + dgap * 2.0f;
    float dsx = m_dialogPanel.x + (dw - dtotal) / 2.0f;
    float dby = m_dialogPanel.y + dh - dbh - 28.0f;
    m_dlgSaveBtn.m_label = "SAVE & EXIT";
    m_dlgSaveBtn.m_rect = {dsx, dby, dbw, dbh};
    m_dlgDiscardBtn.m_label = "DISCARD";
    m_dlgDiscardBtn.m_rect = {dsx + (dbw + dgap), dby, dbw, dbh};
    m_dlgCancelBtn.m_label = "KEEP EDITING";
    m_dlgCancelBtn.m_rect = {dsx + 2.0f * (dbw + dgap), dby, dbw, dbh};
}

void SettingsState::SyncWidgetsFromWorking() {
    m_musicSlider.m_value = m_working.musicVolume;
    m_sfxSlider.m_value = m_working.sfxVolume;
    m_hudScaleSlider.m_value = m_working.hudScale;
}

void SettingsState::PullWorkingFromWidgets() {
    m_working.musicVolume = m_musicSlider.m_value;
    m_working.sfxVolume = m_sfxSlider.m_value;
    m_working.hudScale = m_hudScaleSlider.m_value;
}

// --- Helpers ---------------------------------------------------------------

std::string* SettingsState::WorkingKey(const std::string& action) {
    for (auto& grp : m_working.bindings)
        for (auto& b : grp.bindings)
            if (b.action == action) return &b.key;
    return nullptr;
}

bool SettingsState::IsKeyDuplicated(const std::string& key) const {
    if (key.empty()) return false;
    int count = 0;
    for (auto& grp : m_working.bindings)
        for (auto& b : grp.bindings)
            if (b.key == key) count++;
    return count > 1;
}

bool SettingsState::AnyDuplicates() const {
    for (auto& grp : m_working.bindings)
        for (auto& b : grp.bindings)
            if (IsKeyDuplicated(b.key)) return true;
    return false;
}

int SettingsState::StepFps(int current, int dir) const {
    if (dir > 0) {
        int best = current;
        for (int opt : kFpsOptions)
            if (opt > current) { best = opt; break; }
        return best;
    }
    int best = current;
    int count = static_cast<int>(sizeof(kFpsOptions) / sizeof(kFpsOptions[0]));
    for (int i = count - 1; i >= 0; i--)
        if (kFpsOptions[i] < current) { best = kFpsOptions[i]; break; }
    return best;
}

void SettingsState::ApplyLiveAudio(Game& game, const ConfigValues& v) {
    game.GetSoundSystem().SetMusicVolume(v.musicVolume);
    game.GetSoundSystem().SetSfxVolume(v.sfxVolume);
}

void SettingsState::SetStatus(const std::string& msg) {
    m_status.Set(msg);
}

// --- Actions ---------------------------------------------------------------

void SettingsState::Save(Game& game) {
    PullWorkingFromWidgets();

    // Persist + apply settings.toml values
    GameConfig& cfg = game.GetMutableGameConfig();
    cfg.musicVolume = m_working.musicVolume;
    cfg.sfxVolume = m_working.sfxVolume;
    cfg.fps = m_working.fps;
    cfg.hudScale = m_working.hudScale;
    cfg.Save(game.GetFileStore());

    SetTargetFPS(cfg.fps);
    ApplyLiveAudio(game, m_working);

    // Persist keybindings.json (grouped) and apply each binding live
    nlohmann::json j;
    for (auto& grp : m_working.bindings) {
        for (auto& b : grp.bindings) {
            j[grp.category][b.action] = b.key;
            game.GetInput().AddAction(b.action, Input::ParseKey(b.key));
        }
    }
    game.GetFileStore().SaveJson("config/keybindings.json", j);

    m_snapshot = m_working;
    SetStatus("Settings saved");
}

void SettingsState::Discard(Game& game) {
    m_working = m_snapshot;
    SyncWidgetsFromWorking();
    ApplyLiveAudio(game, m_snapshot);
    SetStatus("Changes discarded");
}

void SettingsState::ResetToDefaults() {
    // Factory defaults: default-constructed config values + canonical keybindings.
    // Stays in the working copy (revertible via Discard) until the user saves.
    ConfigValues def;
    def.bindings = DefaultBindings();
    m_working = def;
    SyncWidgetsFromWorking();
    SetStatus("Reset to defaults (not yet saved)");
}

void SettingsState::RequestBack(Game& game) {
    if (IsDirty())
        m_showDialog = true;
    else
        game.PopState(); // resume whatever pushed us (main menu or the paused match)
}

// --- Keybinding capture ----------------------------------------------------

void SettingsState::BeginRebind(const std::string& action) {
    m_rebinding = true;
    m_rebindAction = action;
    SetStatus("Press a key to bind, ESC to cancel");
}

void SettingsState::HandleRebindCapture(Game& game) {
    (void)game;
    // ESC always cancels the capture rather than binding to Escape.
    if (IsKeyPressed(KEY_ESCAPE)) {
        m_rebinding = false;
        SetStatus("Rebind cancelled");
        return;
    }

    int key = GetKeyPressed();
    if (key == 0) return; // still listening

    std::string name = Input::KeyName(static_cast<KeyboardKey>(key));
    if (name.empty()) {
        m_rebinding = false;
        SetStatus("Unsupported key");
        return;
    }

    if (std::string* slot = WorkingKey(m_rebindAction))
        *slot = name;
    m_rebinding = false;

    // Warn-only: the binding is accepted even if it now collides with another action.
    if (IsKeyDuplicated(name))
        SetStatus("'" + name + "' is bound to multiple actions");
    else
        SetStatus("Bound to " + name);
}

// --- Input -----------------------------------------------------------------

void SettingsState::ProcessInput(Game& game, float /*dt*/) {
    if (m_showDialog) { ProcessDialog(game); return; }
    if (m_rebinding)  { HandleRebindCapture(game); return; }
    ProcessControls(game);
}

void SettingsState::ProcessControls(Game& game) {
    Vector2 mouse = game.GetInput().GetMousePosition();
    bool pressed = game.GetInput().IsMousePressed(MOUSE_LEFT_BUTTON);
    bool down = game.GetInput().IsMouseDown(MOUSE_LEFT_BUTTON);

    // Sliders + live audio preview
    m_musicSlider.Update(mouse, down);
    m_sfxSlider.Update(mouse, down);
    m_hudScaleSlider.Update(mouse, down);
    PullWorkingFromWidgets();
    ApplyLiveAudio(game, m_working);

    // Play a sample sfx when the SFX slider is released so the level is audible.
    bool sfxDragging = m_sfxSlider.IsDragging();
    if (m_sfxWasDragging && !sfxDragging)
        game.GetSoundSystem().PlaySfx("zapper");
    m_sfxWasDragging = sfxDragging;

    // FPS stepper
    m_fpsDownBtn.Update(mouse, pressed);
    m_fpsUpBtn.Update(mouse, pressed);
    if (m_fpsDownBtn.IsClicked()) { game.GetSoundSystem().PlaySfx("button_click"); m_working.fps = StepFps(m_working.fps, -1); }
    if (m_fpsUpBtn.IsClicked())   { game.GetSoundSystem().PlaySfx("button_click"); m_working.fps = StepFps(m_working.fps, +1); }

    // Keybinding cells — clicking one starts capture
    for (auto& row : m_keyRows) {
        row.cell.Update(mouse, pressed);
        if (row.cell.IsClicked()) {
            game.GetSoundSystem().PlaySfx("button_click");
            BeginRebind(row.action);
            return;
        }
    }

    // Bottom buttons
    bool dirty = IsDirty();
    m_saveBtn.Update(mouse, pressed);
    m_discardBtn.Update(mouse, pressed);
    m_resetBtn.Update(mouse, pressed);
    m_backBtn.Update(mouse, pressed);

    if (m_saveBtn.IsClicked() && dirty)    { game.GetSoundSystem().PlaySfx("button_click"); Save(game); }
    if (m_discardBtn.IsClicked() && dirty) { game.GetSoundSystem().PlaySfx("button_click"); Discard(game); }
    if (m_resetBtn.IsClicked())            { game.GetSoundSystem().PlaySfx("button_click"); ResetToDefaults(); }
    if (m_backBtn.IsClicked())             { game.GetSoundSystem().PlaySfx("button_click"); RequestBack(game); }

    // Escape / Cancel acts as Back
    if (game.GetInput().IsPressed("Cancel"))
        RequestBack(game);
}

void SettingsState::ProcessDialog(Game& game) {
    Vector2 mouse = game.GetInput().GetMousePosition();
    bool pressed = game.GetInput().IsMousePressed(MOUSE_LEFT_BUTTON);

    m_dlgSaveBtn.Update(mouse, pressed);
    m_dlgDiscardBtn.Update(mouse, pressed);
    m_dlgCancelBtn.Update(mouse, pressed);

    if (m_dlgSaveBtn.IsClicked()) {
        game.GetSoundSystem().PlaySfx("button_click");
        Save(game);
        m_showDialog = false;
        game.PopState();
        return;
    }
    if (m_dlgDiscardBtn.IsClicked()) {
        game.GetSoundSystem().PlaySfx("button_click");
        m_working = m_snapshot;
        ApplyLiveAudio(game, m_snapshot); // revert any live audio preview
        m_showDialog = false;
        game.PopState();
        return;
    }
    if (m_dlgCancelBtn.IsClicked() || game.GetInput().IsPressed("Cancel")) {
        if (m_dlgCancelBtn.IsClicked()) game.GetSoundSystem().PlaySfx("button_click");
        m_showDialog = false; // keep editing
    }
}

void SettingsState::Update(Game& /*game*/, float dt) {
    m_status.Update(dt);
}

// --- Draw ------------------------------------------------------------------

void SettingsState::Draw(Game& game) {
    float gw = static_cast<float>(game.GetScreen().GetGameWidth());

    ClearBackground(DARKGRAY);
    DrawCenteredText("SETTINGS", gw / 2.0f, 50.0f, 48, RAYWHITE);

    DrawControls(game);

    if (m_showDialog)
        DrawDialog(game);
}

void SettingsState::DrawControls(Game& game) {
    float gw = static_cast<float>(game.GetScreen().GetGameWidth());

    // ----- Left column: audio -----
    Text::Draw("AUDIO", static_cast<int>(kLeftLabelX), static_cast<int>(kAudioHeaderY), 28, kWarnColor, Text::Kind::Heading);

    DrawLabelInRow("Music", kLeftLabelX, kMusicY, kSliderH, 20, RAYWHITE);
    m_musicSlider.Draw();
    DrawLabelInRow(TextFormat("%d%%", static_cast<int>(std::lround(m_working.musicVolume * 100.0f))),
        kValueX, kMusicY, kSliderH, 20, RAYWHITE);

    DrawLabelInRow("SFX", kLeftLabelX, kSfxY, kSliderH, 20, RAYWHITE);
    m_sfxSlider.Draw();
    DrawLabelInRow(TextFormat("%d%%", static_cast<int>(std::lround(m_working.sfxVolume * 100.0f))),
        kValueX, kSfxY, kSliderH, 20, RAYWHITE);

    // ----- Left column: display -----
    Text::Draw("DISPLAY", static_cast<int>(kLeftLabelX), static_cast<int>(kDisplayHdrY), 28, kWarnColor, Text::Kind::Heading);

    DrawLabelInRow("Target FPS", kLeftLabelX, kFpsY, 30.0f, 20, RAYWHITE);
    m_fpsDownBtn.Draw();
    m_fpsDownBtn.DrawLabel(20, RAYWHITE);
    m_fpsUpBtn.Draw();
    m_fpsUpBtn.DrawLabel(20, RAYWHITE);
    DrawCenteredText(TextFormat("%d", m_working.fps),
        m_fpsValueRect.x + m_fpsValueRect.width / 2.0f,
        m_fpsValueRect.y + (m_fpsValueRect.height - 22) / 2.0f, 22, RAYWHITE);

    DrawLabelInRow("HUD Scale", kLeftLabelX, kHudScaleY, kSliderH, 20, RAYWHITE);
    m_hudScaleSlider.Draw();
    DrawLabelInRow(TextFormat("%.2fx", m_working.hudScale),
        kValueX, kHudScaleY, kSliderH, 20, RAYWHITE);

    // ----- Right column: controls -----
    Text::Draw("CONTROLS", static_cast<int>(kCtrlHeaderX), static_cast<int>(kCtrlHeaderY), 28, kWarnColor, Text::Kind::Heading);

    for (auto& gh : m_groupHeaders)
        Text::Draw(gh.category.c_str(), static_cast<int>(kActionLabelX), static_cast<int>(gh.y), 22, SKYBLUE, Text::Kind::Heading);

    for (auto& row : m_keyRows) {
        DrawLabelInRow(row.action.c_str(), kActionLabelX, row.cell.m_rect.y, kKeyCellH, 20, RAYWHITE);
        row.cell.Draw();

        const std::string* key = WorkingKey(row.action);
        bool capturing = m_rebinding && row.action == m_rebindAction;
        const char* cellText = capturing ? "..." : (key && !key->empty() ? key->c_str() : "—");
        Color cellColor = RAYWHITE;
        if (capturing)
            cellColor = SKYBLUE;
        else if (key && IsKeyDuplicated(*key))
            cellColor = kWarnColor;
        DrawCenteredText(cellText,
            row.cell.m_rect.x + row.cell.m_rect.width / 2.0f,
            row.cell.m_rect.y + (kKeyCellH - 20) / 2.0f, 20, cellColor);
    }

    if (AnyDuplicates())
        Text::Draw("! Some keys are bound to multiple actions",
            static_cast<int>(kActionLabelX), static_cast<int>(m_controlsBottomY + 4.0f), 18, kWarnColor);

    // ----- Bottom buttons (Save/Discard greyed when there is nothing to act on) -----
    bool dirty = IsDirty();
    const WidgetStyle& saveStyle = dirty ? kDefaultStyle : kDisabledStyle;
    Color actionableText = dirty ? RAYWHITE : Color{120, 120, 120, 255};

    m_saveBtn.Draw(false, saveStyle);
    m_saveBtn.DrawLabel(18, actionableText);
    m_discardBtn.Draw(false, saveStyle);
    m_discardBtn.DrawLabel(18, actionableText);
    m_resetBtn.Draw();
    m_resetBtn.DrawLabel(18, RAYWHITE);
    m_backBtn.Draw();
    m_backBtn.DrawLabel(18, RAYWHITE);

    // ----- Status toast -----
    m_status.Draw(gw / 2.0f, kBottomBtnY - 36.0f, 20, GREEN);
}

void SettingsState::DrawDialog(Game& game) {
    int gw = game.GetScreen().GetGameWidth();
    int gh = game.GetScreen().GetGameHeight();

    // Dim the whole screen behind the modal.
    DrawRectangle(0, 0, gw, gh, {0, 0, 0, 150});

    DrawRectangleRec(m_dialogPanel, {30, 30, 30, 245});
    DrawRectangleLinesEx(m_dialogPanel, 2.0f, kWarnColor);

    float centerX = m_dialogPanel.x + m_dialogPanel.width / 2.0f;
    DrawCenteredText("UNSAVED CHANGES", centerX, m_dialogPanel.y + 36.0f, 28, RAYWHITE);
    DrawCenteredText("You have unsaved changes. What would you like to do?",
        centerX, m_dialogPanel.y + 84.0f, 18, LIGHTGRAY);

    m_dlgSaveBtn.Draw();
    m_dlgSaveBtn.DrawLabel(18, RAYWHITE);
    m_dlgDiscardBtn.Draw();
    m_dlgDiscardBtn.DrawLabel(18, RAYWHITE);
    m_dlgCancelBtn.Draw();
    m_dlgCancelBtn.DrawLabel(18, RAYWHITE);
}
