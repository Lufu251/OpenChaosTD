#pragma once

#include <states/game_state.hpp>
#include <states/state_ui_helpers.hpp>
#include <engine/systems/ui_widgets.hpp>
#include <string>
#include <vector>

// Settings menu: view, edit and persist the runtime settings (audio volumes, target FPS,
// HUD scale) and rebind controls. Edits live in a working copy; nothing touches disk until
// Save. Safety affordances: Discard, Reset to Defaults, a non-blocking duplicate-key warning,
// and an unsaved-changes dialog when leaving with pending edits.
class SettingsState : public GameState {
public:
    void OnEnter(Game& game) override;
    void OnExit(Game& game) override;

    void ProcessInput(Game& game, float dt) override;
    void Update(Game& game, float dt) override;
    void Draw(Game& game) override;

private:
    // --- Data model ---
    struct Binding {
        std::string action; // e.g. "Up"
        std::string key;    // key name, e.g. "W"; empty if unbound
    };
    struct BindingGroup {
        std::string category; // e.g. "Movement"
        std::vector<Binding> bindings;
    };
    struct ConfigValues {
        float musicVolume = 1.0f;
        float sfxVolume = 1.0f;
        int   fps = 120;
        float hudScale = 1.0f;
        std::vector<BindingGroup> bindings;
        bool operator==(const ConfigValues& o) const;
        bool operator!=(const ConfigValues& o) const { return !(*this == o); }
    };

    // --- Setup / sync ---
    void Layout(Game& game);
    void SyncWidgetsFromWorking(); // push working values into the sliders
    void PullWorkingFromWidgets(); // read slider values back into the working copy
    std::vector<BindingGroup> LoadBindings(Game& game);

    // --- Actions ---
    void Save(Game& game);
    void Discard(Game& game);
    void ResetToDefaults();
    void RequestBack(Game& game);
    void ApplyLiveAudio(Game& game, const ConfigValues& v);
    bool IsDirty() const { return m_working != m_snapshot; }

    // --- Keybindings ---
    void BeginRebind(const std::string& action);
    void HandleRebindCapture(Game& game);
    int  StepFps(int current, int dir) const;
    static int SnapFps(int fps); // nearest kFpsOptions entry, so an off-grid config value is corrected
    std::string* WorkingKey(const std::string& action); // null if action missing
    bool IsKeyDuplicated(const std::string& key) const;  // key bound to >1 action
    bool AnyDuplicates() const;

    // --- Input / draw sub-parts ---
    void ProcessControls(Game& game);
    void ProcessDialog(Game& game);
    void DrawControls(Game& game);
    void DrawDialog(Game& game);
    void SetStatus(const std::string& msg);

    static std::vector<BindingGroup> DefaultBindings();

    // --- State ---
    ConfigValues m_snapshot; // values as loaded / last saved (revert + dirty target)
    ConfigValues m_working;  // values currently being edited

    // Audio + display widgets
    Slider m_musicSlider;
    Slider m_sfxSlider;
    Slider m_hudScaleSlider;
    Button m_fpsDownBtn;
    Button m_fpsUpBtn;
    Rectangle m_fpsValueRect = {};

    // Keybinding rows (built from the working bindings; key text looked up by action)
    struct GroupHeader { std::string category; float y; };
    struct KeyRow { std::string category; std::string action; Button cell; };
    std::vector<GroupHeader> m_groupHeaders;
    std::vector<KeyRow> m_keyRows;
    float m_controlsBottomY = 0.0f;

    // Bottom action buttons
    Button m_saveBtn;
    Button m_discardBtn;
    Button m_resetBtn;
    Button m_backBtn;

    // Unsaved-changes dialog
    bool m_showDialog = false;
    Rectangle m_dialogPanel = {};
    Button m_dlgSaveBtn;
    Button m_dlgDiscardBtn;
    Button m_dlgCancelBtn;

    // Rebind capture
    bool m_rebinding = false;
    std::string m_rebindAction;

    // SFX preview edge detection (play a sample on slider release)
    bool m_sfxWasDragging = false;

    // Transient status toast
    StatusToast m_status;

    static constexpr int kFpsOptions[] = {30, 60, 120, 144, 240};

    // Layout constants (raw virtual coords, matching MenuState's approach)
    static constexpr float kLeftLabelX   = 120.0f;
    static constexpr float kSliderX      = 260.0f;
    static constexpr float kSliderW      = 280.0f;
    static constexpr float kValueX       = 550.0f;
    static constexpr float kAudioHeaderY = 180.0f;
    static constexpr float kMusicY       = 224.0f;
    static constexpr float kSfxY         = 268.0f;
    static constexpr float kDisplayHdrY  = 332.0f;
    static constexpr float kFpsY         = 376.0f;
    static constexpr float kHudScaleY    = 426.0f;
    static constexpr float kSliderH      = 26.0f;

    static constexpr float kCtrlHeaderX  = 660.0f;
    static constexpr float kCtrlHeaderY  = 180.0f;
    static constexpr float kActionLabelX = 670.0f;
    static constexpr float kKeyCellX     = 940.0f;
    static constexpr float kKeyCellW     = 180.0f;
    static constexpr float kKeyCellH     = 30.0f;
    static constexpr float kCtrlTopY     = 224.0f;
    static constexpr float kGroupHeaderH = 32.0f;
    static constexpr float kRowH         = 38.0f;
    static constexpr float kGroupGap     = 12.0f;

    static constexpr float kBottomBtnY   = 1080.0f;
};
