#pragma once
#include <raylib.h>
#include <string>
#include <vector>
#include <engine/core/text_renderer.hpp>
#include <engine/systems/ui_widgets.hpp>
#include <world/desc_line.hpp>

class Input;
class SoundSystem;

// One-shot flag: raised on an event, consumed exactly once.
struct HudSignal {
    void Raise() { m_pending = true; }
    bool Consume() { bool p = m_pending; m_pending = false; return p; }
private:
    bool m_pending = false;
};

void DrawTextCenteredX(const char* text, int centerX, int y, int fontSize, Color color,
                       Text::Kind kind = Text::Kind::Body);

// Base for every HUD component: shared scaling, panel helpers, and visibility state. Concrete
// HUDs expose their own typed ProcessInput/Draw methods fed by read-only views (see hud_views.hpp)
// and an Input& for clicks — they never receive Game or query gameplay state directly.
class HUD {
public:
    virtual ~HUD() = default;

    // Called once on state enter; concrete HUDs call this then do their own layout.
    void Build(float scale) { m_scale = scale; }

    void Show() { m_visible = true; }
    void Hide() { m_visible = false; }
    bool IsVisible() const { return m_visible; }

    void SetSoundSystem(SoundSystem* ss) { m_soundSystem = ss; }

protected:
    float Scaled(float base) const { return base * m_scale; }
    int   ScaledInt(float base) const { return static_cast<int>(base * m_scale); }

    void PlayClickSound() const;

    void DrawPanelBackground(unsigned char alpha, bool border = false) const;
    // Semantic panel-class backgrounds (see hud_theme.hpp): primary windows share one opaque,
    // bordered look; docked bars share a lighter, border-less look. Both build on DrawPanelBackground.
    void DrawWindowBackground() const;
    void DrawDockedBackground() const;
    void ConsumePanelClick(Input& input) const;
    void ClampPanelToScreen(int screenW, int screenH);

    // Shared ProcessInput preamble: returns false (so the caller bails) when hidden;
    // otherwise reads the cursor + left-click state and swallows any panel click.
    bool BeginInput(Input& input, Vector2& mousePos, bool& pressed);
    // Click-only variant for HUDs that just need to swallow clicks on their panel.
    bool BeginInput(Input& input);

    float m_scale = 1.0f;
    bool m_visible = true;
    Rectangle m_panelRect = {};
    SoundSystem* m_soundSystem = nullptr;
};

// ============================================================================
// Read-only views: snapshots handed to the HUD each frame by PlayingState
// ============================================================================

// These carry a snapshot of gameplay state so the HUD never queries GameData / WaveManager /
// factories directly — the HUD only renders these views and raises HudSignals back.

// Top status bar.
struct StatusView {
    int  m_lives = 0;
    int  m_gold = 0;
    int  m_waveNumber = 0;
    int  m_victoryWave = 0;   // 0 = endless
    bool m_waveActive = false;
    bool m_autoSpawn = false;
    int  m_speed = 1;
};

// Bottom tower-build bar. Button names/textures/costs are static config captured at Build time;
// only the current gold changes per frame.
struct BuildBarView {
    int m_gold = 0;
};

// One enemy entry in the WaveHUD "Next Wave" panel (precomputed from a wave group + prototype).
struct WaveEnemyEntry {
    int m_count = 1;
    std::string m_name;
    std::string m_textureKey;
    int m_level = 1;
    bool m_hasProto = false;
    std::vector<DescLine> m_stats; // collected from the prototype's modules' DescribeStats
};

struct WaveView {
    float m_budget = 0.0f;
    std::vector<WaveEnemyEntry> m_entries;
};

// Tower inspection / hover-preview panel. Everything is precomputed by PlayingState so the HUD
// touches no Tower / Enemy / GameData type.
struct TowerInfoView {
    std::string m_name;
    std::string m_description;
    std::vector<DescLine> m_statLines;

    bool m_hasAttack = false;   // false = a wall (no combat UI)
    bool m_interactive = false; // a real selected tower (shows config buttons) vs a hover preview
    bool m_waveActive = false;  // sell shown but disabled mid-wave

    int m_sellRefund = 0;
    int m_level = 0;            // 0-based current level
    int m_upgradeCount = 0;     // number of upgrade tiers (0 = none)

    // Upgrade block (meaningful only when m_upgradeCount > 0)
    bool m_upgradeAtMax = false;
    int  m_upgradeCost = 0;
    bool m_upgradeReady = false; // affordable and not yet max level
    std::vector<DescLine> m_upgradePreview;

    std::string m_targetingName;

    // Placement: screen anchor the panel is positioned against, plus screen bounds for clamping.
    Vector2 m_screenPos = {0.0f, 0.0f};
    int m_screenW = 0;
    int m_screenH = 0;
};

// ============================================================================
// Stateless draw helpers shared across panels
// ============================================================================

// Idioms that recur across panels: a filled+bordered box, a right-aligned label, a column of
// DescLine rows, toggleable/highlight buttons, and an overlay toast. These take all geometry as
// arguments so any panel (or free function) can use them without sharing state.
namespace Hud {

// Fill a rectangle and stroke a 1px border in the given colors.
void DrawFramedBox(Rectangle rect, Color fill, Color border);

// Draw `text` so its right edge sits at `rightEdge` (measure + subtract width).
void DrawTextRightAligned(const char* text, float rightEdge, float y, int fontSize, Color color,
                          Text::Kind kind = Text::Kind::Body);

// Draw each line at (x, y), advancing y by lineH per row. Returns the y past the last line.
float DrawDescLines(const std::vector<DescLine>& lines, float x, float y, float lineH, int fontSize);

// Draw a button that may be disabled: default vs. muted style, with an active-colored label when
// enabled and the shared disabled text color when not.
void DrawToggleableButton(const Button& btn, bool enabled, int fontSize, Color enabledColor);

// Draw a button that toggles between an active highlight (gold selected border + active-colored
// label) and a normal look. Used for stateful toggles like the Status bar Speed/Auto buttons.
void DrawHighlightButton(const Button& btn, bool highlighted, int fontSize,
                         Color activeColor, Color normalColor);

// Draw one ephemeral toast: fill `bg` with the standard overlay panel color and draw `text` at
// (textX, textY), both scaled by `fade` (0..1) so toasts fade out uniformly.
void DrawOverlayToast(const char* text, Rectangle bg, float textX, float textY, int fontSize,
                      float fade);

} // namespace Hud
