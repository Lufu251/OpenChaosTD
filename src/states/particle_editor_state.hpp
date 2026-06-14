#pragma once

#include <states/game_state.hpp>
#include <states/state_ui_helpers.hpp>
#include <engine/systems/ui_widgets.hpp>
#include <engine/systems/particle_system.hpp>
#include <string>
#include <vector>

// Interactive particle preset editor: browse and load presets from
// data/particle_effects.toml, tune every EmitterDesc field live, preview bursts
// and continuous emission at the cursor, and create, save, or delete named
// presets. Saves rewrite the file through toml++ — comments are not preserved.
class ParticleEditorState : public GameState {
public:
    void OnEnter(Game& game) override;
    void OnExit(Game& game) override;

    void ProcessInput(Game& game, float dt) override;
    void Update(Game& game, float dt) override;
    void Draw(Game& game) override;

private:
    // A labeled parameter slider with a numeric readout next to it.
    struct SliderRow {
        const char* m_label = "";
        Slider m_slider;
        bool m_isInt = false;
    };

    // Section header drawn above a group of rows; positioned in Layout.
    struct Header {
        const char* m_text = "";
        Vector2 m_pos = {};
    };

    // --- Setup / sync ---
    void BuildRows();              // labels, ranges, steps; fills m_allRows
    void Layout(Game& game);       // widget rects + headers + preview region
    void SyncWidgetsFromWorking(); // push m_working into the sliders
    void PullWorkingFromWidgets(); // read slider values back into m_working
    void RebuildPresetButtons();   // current browser page worth of buttons
    int  PageCount() const;

    // --- Actions ---
    void LoadPreset(Game& game, int index);
    void NewPreset();              // blank working desc, no selection
    void DeleteSelected(Game& game); // removes the selected preset from file + registry
    void Save(Game& game);
    void SetStatus(const std::string& msg);
    void SanitizeName(); // restrict to [A-Za-z0-9_-] so names are valid bare TOML keys

    // --- Preview ---
    void UpdatePreview(Game& game);
    bool RowEnabled(const SliderRow& row) const; // shape dimension rows gate on m_working.m_shape
    bool AnySliderDragging() const;

    // --- Draw sub-parts ---
    void DrawBrowser();
    void DrawParams();
    void DrawPreview(Game& game);
    void DrawBottomBar(Game& game);
    void DrawRow(const SliderRow& row) const;

    // --- Data model ---
    EmitterDesc m_working;     // the preset being edited (single source of truth)
    EmitterDesc m_lastApplied; // desc baked into the live emitter, for change detection

    // Preset browser
    std::vector<std::string> m_presetNames;
    int m_selectedPreset = -1; // index into m_presetNames; -1 = unsaved/custom
    int m_presetPage = 0;
    int m_presetsPerPage = 1;  // computed in Layout from the available height
    std::vector<Button> m_presetButtons; // one page worth, rebuilt on page flips
    WidgetGroup m_presetGroup;
    Button m_newBtn;
    Button m_deleteBtn; // acts on the selected preset; disabled with no selection
    Button m_pagePrevBtn;
    Button m_pageNextBtn;
    Rectangle m_browserRect = {}; // list area; also the mouse-wheel hover zone

    // Parameter rows
    SliderRow m_countRow, m_emitRateRow, m_lifetimeRow;
    SliderRow m_speedRow, m_speedVarRow, m_spreadRow, m_angleRow;
    SliderRow m_radialRow, m_tangentialRow;
    SliderRow m_sizeRow, m_endSizeRow;
    SliderRow m_startR, m_startG, m_startB, m_startA;
    SliderRow m_endR, m_endG, m_endB, m_endA;
    SliderRow m_shapeWidthRow, m_shapeHeightRow, m_shapeRadiusRow;
    std::vector<SliderRow*> m_allRows; // uniform update/draw iteration

    std::vector<Header> m_headers;
    Rectangle m_startSwatch = {};
    Rectangle m_endSwatch = {};

    // Shape selector — indexed by SpawnShape value
    Button m_shapeButtons[5];
    WidgetGroup m_shapeBtnGroup;

    // Preview region: a state-local particle system, isolated from
    // game.GetParticles(), drawn in virtual screen coords (no camera).
    Rectangle m_previewRect = {};
    ParticleSystem m_previewParticles;
    EmitterHandle m_liveEmitter = DenseSlotMap<Emitter>::INVALID_KEY;
    Toggle m_continuousToggle;
    Button m_clearBtn;

    // Bottom bar
    TextInput m_nameInput;
    Button m_saveBtn;
    Button m_backBtn;

    // Transient status toast
    StatusToast m_status;

    // Layout constants (raw virtual coords, matching SettingsState's approach)
    static constexpr float kTopY       = 110.0f; // content top, below the title
    static constexpr float kMargin     = 40.0f;
    static constexpr float kBrowserX   = 40.0f;
    static constexpr float kBrowserW   = 190.0f;
    static constexpr float kPresetBtnH = 30.0f;
    static constexpr float kPresetGap  = 6.0f;
    static constexpr float kPagerH     = 30.0f;
    static constexpr float kColAX      = 260.0f; // emission/motion/size column
    static constexpr float kColBX      = 550.0f; // colors/shape column
    static constexpr float kLabelW     = 105.0f; // label gutter left of each slider
    // Slider width leaves a value gutter wide enough for 6-char floats (e.g. "270.00") so column
    // A's readouts never run into column B's labels (kColBX - sliderEnd - 10 ~= 70px of value room).
    static constexpr float kSliderW    = 105.0f;
    static constexpr float kSliderH    = 22.0f;
    static constexpr float kRowH       = 30.0f;
    static constexpr float kHeaderH    = 34.0f;
    static constexpr float kGroupGap   = 10.0f;
    static constexpr float kPreviewX   = 860.0f;
    static constexpr float kBottomBarH = 80.0f;  // name/save/back strip above the bottom edge
};
