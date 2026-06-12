#include <states/particle_editor_state.hpp>
#include <states/menu_state.hpp>
#include <engine/core/text_renderer.hpp>
#include <game.hpp>
#include <raylib.h>
#include <algorithm>
#include <cmath>
#include <cctype>
#include <memory>

namespace {
    // Accent tint for section headers and preview hints (matches SettingsState).
    constexpr Color kAccentColor = {255, 180, 0, 255};

    // Color swatch over a dark backing so low-alpha values stay readable.
    void DrawSwatch(Rectangle rect, Color color) {
        DrawRectangleRec(rect, {15, 15, 15, 255});
        DrawRectangleRec(rect, color);
        DrawRectangleLinesEx(rect, 1.0f, {80, 80, 80, 255});
    }

    // Memberwise compare; exact float equality is right here — values only
    // change through slider writes, never through arithmetic drift.
    bool DescEquals(const EmitterDesc& a, const EmitterDesc& b) {
        return ColorEquals(a.m_color, b.m_color) && ColorEquals(a.m_endColor, b.m_endColor) &&
               a.m_count == b.m_count && a.m_speed == b.m_speed &&
               a.m_speedVariance == b.m_speedVariance && a.m_spread == b.m_spread &&
               a.m_angle == b.m_angle && a.m_lifetime == b.m_lifetime &&
               a.m_size == b.m_size && a.m_endSize == b.m_endSize &&
               a.m_shape == b.m_shape && a.m_shapeSize.x == b.m_shapeSize.x &&
               a.m_shapeSize.y == b.m_shapeSize.y && a.m_shapeRadius == b.m_shapeRadius &&
               a.m_radialSpeed == b.m_radialSpeed && a.m_tangentialSpeed == b.m_tangentialSpeed &&
               a.m_emitRate == b.m_emitRate;
    }
}

// --- Lifecycle ---------------------------------------------------------------

void ParticleEditorState::OnEnter(Game& game) {
    m_presetNames = game.GetEmitterPresets().Names();
    BuildRows();
    Layout(game);
    RebuildPresetButtons();

    if (!m_presetNames.empty()) {
        LoadPreset(game, 0);
    } else {
        // Nothing to load — start from defaults with a visible burst count.
        m_working = EmitterDesc{};
        m_working.m_count = 10;
        SyncWidgetsFromWorking();
    }
    m_lastApplied = m_working;
}

void ParticleEditorState::OnExit(Game& /*game*/) {
    m_previewParticles.RemoveEmitter(m_liveEmitter);
    m_previewParticles.Clear();
}

// --- Setup / sync ------------------------------------------------------------

void ParticleEditorState::BuildRows() {
    auto setup = [](SliderRow& row, const char* label, float min, float max, float step, bool isInt) {
        row.m_label = label;
        row.m_slider.m_min = min;
        row.m_slider.m_max = max;
        row.m_slider.m_step = step;
        row.m_isInt = isInt;
    };

    setup(m_countRow, "Count", 0.0f, 200.0f, 1.0f, true);
    setup(m_emitRateRow, "Emit Rate", 0.0f, 200.0f, 1.0f, false);
    setup(m_lifetimeRow, "Lifetime", 0.0f, 5.0f, 0.01f, false);
    setup(m_speedRow, "Speed", 0.0f, 400.0f, 1.0f, false);
    setup(m_speedVarRow, "Variance", 0.0f, 200.0f, 1.0f, false);
    setup(m_spreadRow, "Spread", 0.0f, 360.0f, 1.0f, false);
    setup(m_angleRow, "Angle", 0.0f, 360.0f, 1.0f, false);
    setup(m_radialRow, "Radial", -200.0f, 200.0f, 1.0f, false);
    setup(m_tangentialRow, "Tangential", -200.0f, 200.0f, 1.0f, false);
    setup(m_sizeRow, "Size", 0.0f, 20.0f, 0.1f, false);
    setup(m_endSizeRow, "End Size", 0.0f, 20.0f, 0.1f, false);

    setup(m_startR, "Red", 0.0f, 255.0f, 1.0f, true);
    setup(m_startG, "Green", 0.0f, 255.0f, 1.0f, true);
    setup(m_startB, "Blue", 0.0f, 255.0f, 1.0f, true);
    setup(m_startA, "Alpha", 0.0f, 255.0f, 1.0f, true);
    setup(m_endR, "Red", 0.0f, 255.0f, 1.0f, true);
    setup(m_endG, "Green", 0.0f, 255.0f, 1.0f, true);
    setup(m_endB, "Blue", 0.0f, 255.0f, 1.0f, true);
    setup(m_endA, "Alpha", 0.0f, 255.0f, 1.0f, true);

    setup(m_shapeWidthRow, "Width", 0.0f, 300.0f, 1.0f, false);
    setup(m_shapeHeightRow, "Height", 0.0f, 300.0f, 1.0f, false);
    setup(m_shapeRadiusRow, "Radius", 0.0f, 200.0f, 1.0f, false);

    m_allRows = {&m_countRow, &m_emitRateRow, &m_lifetimeRow,
                 &m_speedRow, &m_speedVarRow, &m_spreadRow, &m_angleRow,
                 &m_radialRow, &m_tangentialRow, &m_sizeRow, &m_endSizeRow,
                 &m_startR, &m_startG, &m_startB, &m_startA,
                 &m_endR, &m_endG, &m_endB, &m_endA,
                 &m_shapeWidthRow, &m_shapeHeightRow, &m_shapeRadiusRow};

    const char* shapeNames[5] = {"POINT", "LINE", "BOX", "CIRCLE", "RING"};
    for (int i = 0; i < 5; i++)
        m_shapeButtons[i].m_label = shapeNames[i];
}

void ParticleEditorState::Layout(Game& game) {
    float gw = static_cast<float>(game.GetScreen().GetGameWidth());
    float gh = static_cast<float>(game.GetScreen().GetGameHeight());

    // Preview fills the right side; its controls and the bottom bar reserve
    // space below, keeping every interactive widget outside the preview rect.
    float bottomBarY = gh - kBottomBarH;
    float controlRowH = 40.0f;
    m_previewRect = {kPreviewX, kTopY, gw - kMargin - kPreviewX,
                     bottomBarY - controlRowH - 20.0f - kTopY};

    float controlY = m_previewRect.y + m_previewRect.height + 10.0f;
    m_continuousToggle.m_label = "Follow cursor";
    m_continuousToggle.m_rect = {kPreviewX, controlY, 26.0f, 26.0f};
    m_clearBtn.m_label = "CLEAR";
    m_clearBtn.m_rect = {kPreviewX + m_previewRect.width - 90.0f, controlY, 90.0f, 26.0f};

    // Preset browser: new/delete actions under the header, then as many name
    // buttons as fit, then the pager underneath.
    m_newBtn.m_label = "NEW";
    m_newBtn.m_rect = {kBrowserX, kTopY + 36.0f, 92.0f, 30.0f};
    m_deleteBtn.m_label = "DELETE";
    m_deleteBtn.m_rect = {kBrowserX + kBrowserW - 92.0f, kTopY + 36.0f, 92.0f, 30.0f};

    float listTop = kTopY + 76.0f;
    float listBottom = m_previewRect.y + m_previewRect.height - kPagerH - 10.0f;
    m_presetsPerPage = std::max(1, static_cast<int>((listBottom - listTop) / (kPresetBtnH + kPresetGap)));
    m_browserRect = {kBrowserX, listTop, kBrowserW, listBottom - listTop};
    m_pagePrevBtn.m_label = "<";
    m_pagePrevBtn.m_rect = {kBrowserX, listBottom + 6.0f, 40.0f, kPagerH};
    m_pageNextBtn.m_label = ">";
    m_pageNextBtn.m_rect = {kBrowserX + kBrowserW - 40.0f, listBottom + 6.0f, 40.0f, kPagerH};

    // Parameter columns: rows flow downward under their group headers.
    m_headers.clear();
    auto placeHeader = [&](const char* text, float x, float& y) {
        m_headers.push_back({text, {x, y}});
        y += kHeaderH;
    };
    auto placeRow = [&](SliderRow& row, float x, float& y) {
        row.m_slider.m_rect = {x + kLabelW, y, kSliderW, kSliderH};
        y += kRowH;
    };

    // Column A: emission, motion, size
    float y = kTopY;
    placeHeader("EMISSION", kColAX, y);
    placeRow(m_countRow, kColAX, y);
    placeRow(m_emitRateRow, kColAX, y);
    placeRow(m_lifetimeRow, kColAX, y);
    y += kGroupGap;
    placeHeader("MOTION", kColAX, y);
    placeRow(m_speedRow, kColAX, y);
    placeRow(m_speedVarRow, kColAX, y);
    placeRow(m_spreadRow, kColAX, y);
    placeRow(m_angleRow, kColAX, y);
    placeRow(m_radialRow, kColAX, y);
    placeRow(m_tangentialRow, kColAX, y);
    y += kGroupGap;
    placeHeader("SIZE", kColAX, y);
    placeRow(m_sizeRow, kColAX, y);
    placeRow(m_endSizeRow, kColAX, y);

    // Column B: colors (with swatches beside the headers), then shape
    y = kTopY;
    placeHeader("START COLOR", kColBX, y);
    m_startSwatch = {kColBX + 175.0f, y - kHeaderH + 2.0f, 80.0f, 24.0f};
    placeRow(m_startR, kColBX, y);
    placeRow(m_startG, kColBX, y);
    placeRow(m_startB, kColBX, y);
    placeRow(m_startA, kColBX, y);
    y += kGroupGap;
    placeHeader("END COLOR", kColBX, y);
    m_endSwatch = {kColBX + 175.0f, y - kHeaderH + 2.0f, 80.0f, 24.0f};
    placeRow(m_endR, kColBX, y);
    placeRow(m_endG, kColBX, y);
    placeRow(m_endB, kColBX, y);
    placeRow(m_endA, kColBX, y);
    y += kGroupGap;
    placeHeader("SHAPE", kColBX, y);
    for (int i = 0; i < 5; i++)
        m_shapeButtons[i].m_rect = {kColBX + i * 56.0f, y, 52.0f, 26.0f};
    y += kRowH;
    placeRow(m_shapeWidthRow, kColBX, y);
    placeRow(m_shapeHeightRow, kColBX, y);
    placeRow(m_shapeRadiusRow, kColBX, y);

    // Bottom bar: name input + save + back
    float barY = bottomBarY + 18.0f;
    m_nameInput.m_rect = {kBrowserX + 70.0f, barY, 280.0f, 40.0f};
    m_saveBtn.m_rect = {kBrowserX + 370.0f, barY, 160.0f, 40.0f};
    m_backBtn.m_label = "BACK";
    m_backBtn.m_rect = {kBrowserX + 550.0f, barY, 120.0f, 40.0f};
}

void ParticleEditorState::SyncWidgetsFromWorking() {
    m_countRow.m_slider.m_value = static_cast<float>(m_working.m_count);
    m_emitRateRow.m_slider.m_value = m_working.m_emitRate;
    m_lifetimeRow.m_slider.m_value = m_working.m_lifetime;
    m_speedRow.m_slider.m_value = m_working.m_speed;
    m_speedVarRow.m_slider.m_value = m_working.m_speedVariance;
    m_spreadRow.m_slider.m_value = m_working.m_spread;
    m_angleRow.m_slider.m_value = m_working.m_angle;
    m_radialRow.m_slider.m_value = m_working.m_radialSpeed;
    m_tangentialRow.m_slider.m_value = m_working.m_tangentialSpeed;
    m_sizeRow.m_slider.m_value = m_working.m_size;
    m_endSizeRow.m_slider.m_value = m_working.m_endSize;
    m_startR.m_slider.m_value = static_cast<float>(m_working.m_color.r);
    m_startG.m_slider.m_value = static_cast<float>(m_working.m_color.g);
    m_startB.m_slider.m_value = static_cast<float>(m_working.m_color.b);
    m_startA.m_slider.m_value = static_cast<float>(m_working.m_color.a);
    m_endR.m_slider.m_value = static_cast<float>(m_working.m_endColor.r);
    m_endG.m_slider.m_value = static_cast<float>(m_working.m_endColor.g);
    m_endB.m_slider.m_value = static_cast<float>(m_working.m_endColor.b);
    m_endA.m_slider.m_value = static_cast<float>(m_working.m_endColor.a);
    m_shapeWidthRow.m_slider.m_value = m_working.m_shapeSize.x;
    m_shapeHeightRow.m_slider.m_value = m_working.m_shapeSize.y;
    m_shapeRadiusRow.m_slider.m_value = m_working.m_shapeRadius;
}

void ParticleEditorState::PullWorkingFromWidgets() {
    auto asByte = [](float v) {
        return static_cast<unsigned char>(std::lround(v));
    };
    m_working.m_count = static_cast<int>(std::lround(m_countRow.m_slider.m_value));
    m_working.m_emitRate = m_emitRateRow.m_slider.m_value;
    m_working.m_lifetime = m_lifetimeRow.m_slider.m_value;
    m_working.m_speed = m_speedRow.m_slider.m_value;
    m_working.m_speedVariance = m_speedVarRow.m_slider.m_value;
    m_working.m_spread = m_spreadRow.m_slider.m_value;
    m_working.m_angle = m_angleRow.m_slider.m_value;
    m_working.m_radialSpeed = m_radialRow.m_slider.m_value;
    m_working.m_tangentialSpeed = m_tangentialRow.m_slider.m_value;
    m_working.m_size = m_sizeRow.m_slider.m_value;
    m_working.m_endSize = m_endSizeRow.m_slider.m_value;
    m_working.m_color = {asByte(m_startR.m_slider.m_value), asByte(m_startG.m_slider.m_value),
                         asByte(m_startB.m_slider.m_value), asByte(m_startA.m_slider.m_value)};
    m_working.m_endColor = {asByte(m_endR.m_slider.m_value), asByte(m_endG.m_slider.m_value),
                            asByte(m_endB.m_slider.m_value), asByte(m_endA.m_slider.m_value)};
    m_working.m_shapeSize = {m_shapeWidthRow.m_slider.m_value, m_shapeHeightRow.m_slider.m_value};
    m_working.m_shapeRadius = m_shapeRadiusRow.m_slider.m_value;
}

int ParticleEditorState::PageCount() const {
    if (m_presetNames.empty()) return 1;
    return (static_cast<int>(m_presetNames.size()) + m_presetsPerPage - 1) / m_presetsPerPage;
}

void ParticleEditorState::RebuildPresetButtons() {
    m_presetButtons.clear();
    int start = m_presetPage * m_presetsPerPage;
    int end = std::min(static_cast<int>(m_presetNames.size()), start + m_presetsPerPage);
    float y = m_browserRect.y;
    for (int i = start; i < end; i++) {
        Button b;
        b.m_label = m_presetNames[i];
        b.m_rect = {m_browserRect.x, y, m_browserRect.width, kPresetBtnH};
        m_presetButtons.push_back(b);
        y += kPresetBtnH + kPresetGap;
    }
}

// --- Helpers -----------------------------------------------------------------

bool ParticleEditorState::RowEnabled(const SliderRow& row) const {
    // Shape dimension rows only matter for the shapes that read them; the
    // values still live in m_working and round-trip regardless.
    if (&row == &m_shapeWidthRow)
        return m_working.m_shape == SpawnShape::Line || m_working.m_shape == SpawnShape::Box;
    if (&row == &m_shapeHeightRow)
        return m_working.m_shape == SpawnShape::Box;
    if (&row == &m_shapeRadiusRow)
        return m_working.m_shape == SpawnShape::Circle || m_working.m_shape == SpawnShape::Ring;
    return true;
}

bool ParticleEditorState::AnySliderDragging() const {
    for (const SliderRow* row : m_allRows)
        if (row->m_slider.IsDragging()) return true;
    return false;
}

void ParticleEditorState::SetStatus(const std::string& msg) {
    m_status.Set(msg);
}

void ParticleEditorState::SanitizeName() {
    // TextInput accepts any printable char; preset names must stay valid bare
    // TOML keys so the saved table round-trips.
    std::string& t = m_nameInput.m_text;
    t.erase(std::remove_if(t.begin(), t.end(), [](unsigned char c) {
        return !(std::isalnum(c) || c == '_' || c == '-');
    }), t.end());
}

// --- Actions -----------------------------------------------------------------

void ParticleEditorState::LoadPreset(Game& game, int index) {
    if (index < 0 || index >= static_cast<int>(m_presetNames.size())) return;
    m_selectedPreset = index;
    m_working = game.GetEmitterPresets().Get(m_presetNames[index]);
    m_nameInput.m_text = m_presetNames[index];
    SyncWidgetsFromWorking();
    // A running live emitter picks up the change via UpdatePreview's desc compare.
}

void ParticleEditorState::NewPreset() {
    // Blank slate: defaults plus a visible burst count, no selection, no name.
    m_working = EmitterDesc{};
    m_working.m_count = 10;
    m_selectedPreset = -1;
    m_nameInput.m_text.clear();
    SyncWidgetsFromWorking();
    SetStatus("New preset - name it and save");
}

void ParticleEditorState::DeleteSelected(Game& game) {
    const std::string name = m_presetNames[m_selectedPreset];
    if (!game.GetEmitterPresets().DeletePreset(game.GetFileStore(), name)) {
        SetStatus("Delete failed - see log");
        return;
    }

    // The deleted values stay loaded in the editor (name included), so a
    // mistaken delete is one SAVE click away from being undone.
    m_presetNames = game.GetEmitterPresets().Names();
    m_selectedPreset = -1;
    m_presetPage = std::clamp(m_presetPage, 0, PageCount() - 1);
    RebuildPresetButtons();
    SetStatus("Deleted '" + name + "'");
}

void ParticleEditorState::Save(Game& game) {
    const std::string name = m_nameInput.m_text;
    if (name.empty()) {
        SetStatus("Enter a preset name");
        return;
    }
    if (!game.GetEmitterPresets().SavePreset(game.GetFileStore(), name, m_working)) {
        SetStatus("Save failed - see log");
        return;
    }

    // Refresh the browser and select the saved preset on its page.
    m_presetNames = game.GetEmitterPresets().Names();
    auto it = std::find(m_presetNames.begin(), m_presetNames.end(), name);
    m_selectedPreset = static_cast<int>(it - m_presetNames.begin());
    m_presetPage = m_selectedPreset / m_presetsPerPage;
    RebuildPresetButtons();
    SetStatus("Saved '" + name + "'");
}

// --- Input -------------------------------------------------------------------

void ParticleEditorState::ProcessInput(Game& game, float /*dt*/) {
    Input& input = game.GetInput();
    Vector2 mouse = input.GetMousePosition();
    bool pressed = input.IsMousePressed(MOUSE_LEFT_BUTTON);
    bool down = input.IsMouseDown(MOUSE_LEFT_BUTTON);

    // Parameter sliders (shape dimension rows only while their shape uses them)
    for (SliderRow* row : m_allRows)
        if (RowEnabled(*row))
            row->m_slider.Update(mouse, down);
    PullWorkingFromWidgets();

    // Shape selector
    for (int i = 0; i < 5; i++) {
        m_shapeButtons[i].Update(mouse, pressed);
        if (m_shapeButtons[i].IsClicked()) {
            game.GetSoundSystem().PlaySfx("button_click");
            m_working.m_shape = static_cast<SpawnShape>(i);
        }
    }

    // Preset management
    m_newBtn.Update(mouse, pressed);
    m_deleteBtn.Update(mouse, pressed);
    if (m_newBtn.IsClicked()) {
        game.GetSoundSystem().PlaySfx("button_click");
        NewPreset();
    }
    if (m_deleteBtn.IsClicked() && m_selectedPreset >= 0) {
        game.GetSoundSystem().PlaySfx("button_click");
        DeleteSelected(game);
    }

    // Preset browser: pager via buttons or mouse wheel over the list
    m_pagePrevBtn.Update(mouse, pressed);
    m_pageNextBtn.Update(mouse, pressed);
    int pageStep = 0;
    if (m_pagePrevBtn.IsClicked()) { game.GetSoundSystem().PlaySfx("button_click"); pageStep = -1; }
    if (m_pageNextBtn.IsClicked()) { game.GetSoundSystem().PlaySfx("button_click"); pageStep = +1; }
    float wheel = input.GetMouseWheelDelta();
    if (wheel != 0.0f && CheckCollisionPointRec(mouse, m_browserRect))
        pageStep = wheel > 0.0f ? -1 : +1;
    if (pageStep != 0) {
        m_presetPage = std::clamp(m_presetPage + pageStep, 0, PageCount() - 1);
        RebuildPresetButtons();
    }

    for (size_t i = 0; i < m_presetButtons.size(); i++) {
        m_presetButtons[i].Update(mouse, pressed);
        if (m_presetButtons[i].IsClicked()) {
            game.GetSoundSystem().PlaySfx("button_click");
            LoadPreset(game, m_presetPage * m_presetsPerPage + static_cast<int>(i));
        }
    }

    // Preview controls
    m_continuousToggle.Update(mouse, pressed);
    m_clearBtn.Update(mouse, pressed);
    if (m_clearBtn.IsClicked()) {
        game.GetSoundSystem().PlaySfx("button_click");
        m_previewParticles.Clear(); // wipes live emitters too — re-added next frame
        m_liveEmitter = DenseSlotMap<Emitter>::INVALID_KEY;
    }

    // Bottom bar
    m_nameInput.Update(mouse, pressed);
    SanitizeName();
    m_saveBtn.m_label = game.GetEmitterPresets().Has(m_nameInput.m_text) ? "OVERWRITE" : "SAVE";
    m_saveBtn.Update(mouse, pressed);
    m_backBtn.Update(mouse, pressed);

    if (m_saveBtn.IsClicked() && !m_nameInput.m_text.empty()) {
        game.GetSoundSystem().PlaySfx("button_click");
        Save(game);
    }

    // Escape acts as Back, but not while typing a preset name
    if (m_backBtn.IsClicked() || (input.IsPressed("Cancel") && !m_nameInput.IsFocused())) {
        if (m_backBtn.IsClicked()) game.GetSoundSystem().PlaySfx("button_click");
        game.ChangeState(std::make_unique<MenuState>());
        return;
    }

    UpdatePreview(game);
}

void ParticleEditorState::UpdatePreview(Game& game) {
    Input& input = game.GetInput();
    Vector2 mouse = input.GetMousePosition();

    // Gate world-style interactions to the preview area so widget clicks and
    // slider drags never double as bursts.
    bool cursorInPreview = CheckCollisionPointRec(mouse, m_previewRect) &&
                           !AnySliderDragging() && !input.IsMouseInputConsumed();

    // Manual burst
    if (cursorInPreview && input.IsMousePressed(MOUSE_LEFT_BUTTON))
        m_previewParticles.Emit(mouse, m_working);

    // Continuous emission follows the cursor, pinned inside the preview
    if (m_continuousToggle.m_value) {
        Vector2 anchor = {
            std::clamp(mouse.x, m_previewRect.x, m_previewRect.x + m_previewRect.width),
            std::clamp(mouse.y, m_previewRect.y, m_previewRect.y + m_previewRect.height)};

        // AddEmitter copies the desc, so parameter edits require a re-add.
        bool missing = m_liveEmitter == DenseSlotMap<Emitter>::INVALID_KEY;
        if (missing || !DescEquals(m_working, m_lastApplied)) {
            m_previewParticles.RemoveEmitter(m_liveEmitter);
            m_liveEmitter = m_previewParticles.AddEmitter(m_working, anchor);
            m_lastApplied = m_working;
        }
        m_previewParticles.UpdateEmitter(m_liveEmitter, anchor); // keepalive refresh
    } else if (m_liveEmitter != DenseSlotMap<Emitter>::INVALID_KEY) {
        m_previewParticles.RemoveEmitter(m_liveEmitter);
        m_liveEmitter = DenseSlotMap<Emitter>::INVALID_KEY;
    }
}

void ParticleEditorState::Update(Game& /*game*/, float dt) {
    m_previewParticles.Tick(dt);
    m_status.Update(dt);
}

// --- Draw ----------------------------------------------------------------------

void ParticleEditorState::Draw(Game& game) {
    float gw = static_cast<float>(game.GetScreen().GetGameWidth());

    ClearBackground(DARKGRAY);
    DrawCenteredText("PARTICLE EDITOR", gw / 2.0f, 40.0f, 40, RAYWHITE);

    DrawBrowser();
    DrawParams();
    DrawPreview(game);
    DrawBottomBar(game);
}

void ParticleEditorState::DrawBrowser() {
    Text::Draw("PRESETS", static_cast<int>(kBrowserX), static_cast<int>(kTopY), 28, kAccentColor);

    m_newBtn.Draw();
    m_newBtn.DrawLabel(14, RAYWHITE);
    bool canDelete = m_selectedPreset >= 0;
    m_deleteBtn.Draw(false, canDelete ? kDefaultStyle : kDisabledStyle);
    m_deleteBtn.DrawLabel(14, canDelete ? RAYWHITE : Color{120, 120, 120, 255});

    if (m_presetNames.empty()) {
        Text::Draw("No presets found", static_cast<int>(kBrowserX),
            static_cast<int>(m_browserRect.y), 16, LIGHTGRAY);
        return;
    }

    int start = m_presetPage * m_presetsPerPage;
    for (size_t i = 0; i < m_presetButtons.size(); i++) {
        bool selected = start + static_cast<int>(i) == m_selectedPreset;
        m_presetButtons[i].Draw(selected);
        m_presetButtons[i].DrawLabel(14, RAYWHITE);
    }

    if (PageCount() > 1) {
        m_pagePrevBtn.Draw();
        m_pagePrevBtn.DrawLabel(16, RAYWHITE);
        m_pageNextBtn.Draw();
        m_pageNextBtn.DrawLabel(16, RAYWHITE);
        DrawCenteredText(TextFormat("%d/%d", m_presetPage + 1, PageCount()),
            kBrowserX + kBrowserW / 2.0f, m_pagePrevBtn.m_rect.y + 7.0f, 16, RAYWHITE);
    }
}

void ParticleEditorState::DrawRow(const SliderRow& row) const {
    bool enabled = RowEnabled(row);
    const WidgetStyle& style = enabled ? kDefaultStyle : kDisabledStyle;
    Color textColor = enabled ? RAYWHITE : Color{100, 100, 100, 255};

    const Rectangle& r = row.m_slider.m_rect;
    DrawLabelInRow(row.m_label, r.x - kLabelW, r.y, r.height, 16, textColor);
    row.m_slider.Draw(style);

    const char* value = row.m_isInt
        ? TextFormat("%d", static_cast<int>(std::lround(row.m_slider.m_value)))
        : TextFormat("%.2f", row.m_slider.m_value);
    DrawLabelInRow(value, r.x + r.width + 10.0f, r.y, r.height, 16, textColor);
}

void ParticleEditorState::DrawParams() {
    for (const Header& h : m_headers)
        Text::Draw(h.m_text, static_cast<int>(h.m_pos.x), static_cast<int>(h.m_pos.y), 24, kAccentColor);

    for (const SliderRow* row : m_allRows)
        DrawRow(*row);

    DrawSwatch(m_startSwatch, m_working.m_color);
    DrawSwatch(m_endSwatch, m_working.m_endColor);

    for (int i = 0; i < 5; i++) {
        bool selected = m_working.m_shape == static_cast<SpawnShape>(i);
        m_shapeButtons[i].Draw(selected);
        m_shapeButtons[i].DrawLabel(12, RAYWHITE);
    }
}

void ParticleEditorState::DrawPreview(Game& game) {
    // Near-black backdrop so bright particles read well; clip to the rect.
    DrawRectangleRec(m_previewRect, {14, 14, 18, 255});
    game.GetScreen().BeginScissor(m_previewRect);
    m_previewParticles.Draw();
    game.GetScreen().EndScissor();
    DrawRectangleLinesEx(m_previewRect, 1.0f, {80, 80, 80, 255});

    float cx = m_previewRect.x + m_previewRect.width / 2.0f;
    DrawCenteredText("Click to burst", cx, m_previewRect.y + 8.0f, 14, GRAY);
    if (m_working.m_count <= 0)
        DrawCenteredText("count is 0 - bursts spawn nothing", cx, m_previewRect.y + 28.0f, 14, kAccentColor);
    else if (m_continuousToggle.m_value && m_working.m_emitRate <= 0.0f)
        DrawCenteredText("emitRate is 0 - no continuous emission", cx, m_previewRect.y + 28.0f, 14, kAccentColor);

    m_continuousToggle.Draw();
    m_clearBtn.Draw();
    m_clearBtn.DrawLabel(14, RAYWHITE);
}

void ParticleEditorState::DrawBottomBar(Game& game) {
    float barY = m_nameInput.m_rect.y;
    DrawLabelInRow("NAME", kBrowserX, barY, m_nameInput.m_rect.height, 20, RAYWHITE);
    m_nameInput.Draw();

    bool canSave = !m_nameInput.m_text.empty();
    m_saveBtn.Draw(false, canSave ? kDefaultStyle : kDisabledStyle);
    m_saveBtn.DrawLabel(18, canSave ? RAYWHITE : Color{120, 120, 120, 255});
    m_backBtn.Draw();
    m_backBtn.DrawLabel(18, RAYWHITE);

    m_status.Draw(static_cast<float>(game.GetScreen().GetGameWidth()) / 2.0f, barY - 26.0f, 18, GREEN);
}
