#pragma once

#include <raylib.h>

class FileStore;

// Shared HUD theme: the single source of truth for panel colors, the semantic typographic scale,
// the panel-class background standards, the sibling-layout anchor (status-bar height), and the
// per-panel layout metrics that several HUDs would otherwise each re-declare and re-scale. Keeping
// these here means a palette, DPI, or typography change touches one file.
//
// All constants are non-const inline variables so they can be overridden at runtime by
// LoadConfig(). HUD draw code continues to reference them by name — nothing else changes.
namespace Hud {

// --- Palette ----------------------------------------------------------------
// Panel chrome.
inline Color kPanelBorder  {80, 80, 80, 255};
// Secondary / dim body text (descriptions, budgets, placeholders).
inline Color kTextMuted    {180, 180, 180, 255};
// Upgrade button label when the purchase is affordable.
inline Color kUpgradeReady {160, 240, 120, 255};
// Upgrade tooltip border (gold accent).
inline Color kTooltipBorder{255, 180, 0, 255};
// Wave enemy card frame.
inline Color kCardFill     {40, 40, 48, 200};
inline Color kCardBorder   {90, 90, 100, 255};
// Status-bar endless-mode infinity glyph.
inline Color kInfinityGlyph{120, 180, 220, 255};

// Semantic accents — named so concrete draw methods never reach for raw raylib color literals.
inline Color kHighlight       {255, 203, 0,   255}; // active toggle (Speed/Auto) — GOLD
inline Color kCostAffordable  {0,   228, 48,  255}; // build cost the player can pay — GREEN
inline Color kCostUnaffordable{230, 41,  55,  255}; // build cost too expensive — RED
inline Color kSellLabel       {0,   228, 48,  255}; // sell button label — GREEN
inline Color kTargetLabel     {102, 191, 255, 255}; // retarget button label — SKYBLUE

// RGB bases for the two dynamic-alpha helpers below; alpha is ignored and set per call.
inline Color kPanelBgRgb   {20,  20,  20,  255};
inline Color kEventTextRgb {255, 220, 80,  255};

// The dark panel fill is shared but drawn at varying opacity, so it is a helper rather than a
// constant. The event toast text likewise fades, so its RGB lives behind a helper too.
inline Color PanelBg(unsigned char alpha)   { return {kPanelBgRgb.r,   kPanelBgRgb.g,   kPanelBgRgb.b,   alpha}; }
inline Color EventText(unsigned char alpha) { return {kEventTextRgb.r, kEventTextRgb.g, kEventTextRgb.b, alpha}; }

// --- Panel-class background standards ---------------------------------------
// Every panel falls into one of three semantic classes; each class has one opacity (and border
// rule), so panels never pick an ad-hoc alpha. See HUD::DrawWindowBackground / DrawDockedBackground.
inline unsigned char kPanelAlphaWindow = 220; // Primary windows: Pause, TowerInfo, Wave (+ border)
inline unsigned char kPanelAlphaDocked = 200; // Docked bars: Status, Build (no border)
inline unsigned char kOverlayBgAlpha   = 160; // Ephemeral toast background, before the fade factor
inline unsigned char kOverlayTextAlpha = 220; // Ephemeral toast text, before the fade factor
inline unsigned char kTooltipBgAlpha   = 235; // Upgrade tooltip fill
inline Color kScreenDim {0, 0, 0, 120};       // Full-screen dim behind the modal pause menu

// --- Semantic typographic scale ---------------------------------------------
// Concrete HUDs request a role rather than a raw font-size integer; every panel that shows the same
// kind of text therefore renders at the same scaled size. Bases are unscaled; FontSize() applies DPI.
enum class FontRole { Title, Header, Body, ButtonLabel, Small };
inline float kFontTitleBase       = 28.0f; // modal title (PAUSED)
inline float kFontHeaderBase      = 14.0f; // panel headers, status readout
inline float kFontBodyBase        = 11.0f; // descriptions, stat rows, card text
inline float kFontButtonLabelBase = 12.0f; // button labels
inline float kFontSmallBase       = 8.0f;  // compact build-bar name/cost

inline int FontSize(FontRole role, float scale) {
    float base = kFontBodyBase;
    switch (role) {
        case FontRole::Title:       base = kFontTitleBase; break;
        case FontRole::Header:      base = kFontHeaderBase; break;
        case FontRole::Body:        base = kFontBodyBase; break;
        case FontRole::ButtonLabel: base = kFontButtonLabelBase; break;
        case FontRole::Small:       base = kFontSmallBase; break;
    }
    return static_cast<int>(base * scale);
}

// --- Full-screen state UI ---------------------------------------------------
// Menu, settings, end and datapack-select screens draw through these so a palette or title-size
// change propagates the same way HUD panels do. These sizes are fixed (full-screen states do not
// apply hudScale). Defaults match the previous hardcoded literals so removal of the file is safe.
inline float kFontStateTitle  = 40.0f; // main-menu / datapack-select screen title
inline float kFontScreenTitle = 48.0f; // settings header, end-screen result title
inline float kFontMenuButton  = 24.0f; // menu / end / back button labels

inline Color kStateBackground{80,  80,  80,  255}; // full-screen clear (was raylib DARKGRAY)
inline Color kDialogBg       {30,  30,  30,  245}; // modal dialog panel fill
inline Color kDialogOverlay  {0,   0,   0,   150}; // screen dim behind a modal dialog
inline Color kDisabledText   {120, 120, 120, 255}; // greyed action-button label text
inline Color kWarning        {255, 180, 0,   255}; // section headers, conflict highlight, dialog border
inline Color kPlaceholderBg  {20,  20,  25,  255}; // missing-icon fill and scrollbar track
inline Color kSubtle         {160, 160, 170, 255}; // secondary text on selection screens
inline Color kVictory        {255, 203, 0,   255}; // end-screen win title
inline Color kDefeat         {230, 41,  55,  255}; // end-screen loss title

// --- Sibling layout anchor --------------------------------------------------
// Unscaled height of the top StatusHUD bar. EventHUD and WaveHUD position their content below the
// bar; reading this (then applying their own Scaled()) keeps them in sync if the bar height changes.
inline float kStatusBarBaseHeight = 36.0f;
// Extra gap WaveHUD leaves below the status bar.
inline float kWavePanelGap = 6.0f;

// --- Shared panel metrics ---------------------------------------------------
// Unscaled bases shared by every text panel, so line height and margins no longer diverge per panel.
inline float kMarginBase  = 8.0f;
inline float kLineHBase   = 14.0f;
inline float kHeaderHBase = 20.0f;

// --- Derived layout ratios --------------------------------------------------
// Offsets that are pure functions of the typographic scale live here as dimensionless ratios
// rather than as standalone pixel knobs in config. Each is written as (former pixel value / base)
// so the default font bases reproduce the previous hand-tuned layout exactly; change a font and the
// dependent geometry tracks it. A panel multiplies the relevant base by the ratio, then applies its
// usual Scaled(). The pause button spacing and title offset fall out of the bases directly (button
// height plus one label-font gap; title baseline one title-font down) and need no constant.
inline constexpr float kPauseBtnHToLabel     = 44.0f / 12.0f; // pause button height / label font (44)
inline constexpr float kPauseFirstBtnToTitle = 80.0f / 28.0f; // first pause button Y / title font (80)
inline constexpr float kStatusBtnHToLabel    = 24.0f / 12.0f; // status button height / label font (24)
inline constexpr float kBuildIconYToBtn      = 8.0f  / 64.0f; // build icon nudge / button size (8)
inline constexpr float kBuildNameYToBtn      = 18.0f / 64.0f; // build name baseline / button size (18)
inline constexpr float kBuildCostYToBtn      = 9.0f  / 64.0f; // build cost baseline / button size (9)
inline constexpr float kInfoDescLineToBody   = 13.0f / 11.0f; // info description line height / body font (13)
inline constexpr float kInfoSellHToBody      = 22.0f / 11.0f; // info action button height / body font (22)
inline constexpr float kInfoSellGapToMargin  = 6.0f  / 8.0f;  // info button gap / margin (6)
inline constexpr float kToastRowToBody       = 20.0f / 11.0f; // event toast row stride / body font (20)

// --- Per-panel layout config ------------------------------------------------
// Unscaled base dimensions for each HUD panel. LoadConfig() overwrites these from config/hud.toml;
// the defaults match the previous hardcoded values so removal of the file is safe.

struct PausePanelCfg {
    float width  = 240.0f;
    float height = 460.0f;
    float btnW   = 180.0f;
    // btnH, btnSpacing, titleOff and firstBtnY are derived from the typographic scale — see the
    // derived layout ratios below.
};
struct StatusPanelCfg {
    float waveW  = 90.0f;
    float autoW  = 48.0f;
    float wavesW = 56.0f;
    float margin = 6.0f;
    // btnH is derived from the label font — see the derived layout ratios below.
};
struct WavePanelCfg {
    float width       = 200.0f;
    float cardGap     = 6.0f;
    float cardPadding = 6.0f;
    float iconSize    = 44.0f;
};
struct BuildPanelCfg {
    float btnSize = 64.0f;
    float panelH  = 80.0f;
    float gap     = 4.0f;
    // iconY, nameY and costY are derived from the button size — see the derived layout ratios below.
};
struct InfoPanelCfg {
    float width     = 160.0f;
    float anchorGap = 20.0f;
    // descLineH, sellH and sellGap are derived from the body font / margin — see below.
};
struct EventCfg {
    int   maxEntries = 5;
    float fadeTime   = 1.0f;
};

inline PausePanelCfg  g_pauseCfg;
inline StatusPanelCfg g_statusCfg;
inline WavePanelCfg   g_waveCfg;
inline BuildPanelCfg  g_buildCfg;
inline InfoPanelCfg   g_infoCfg;
inline EventCfg       g_eventCfg;

// Load all theme constants and per-panel configs from config/hud.toml.
// Safe to skip — all variables retain their default values if the file is absent.
void LoadConfig(FileStore& fileStore);

// The common set of scaled layout values every HUD flows through. Make() resolves the shared margins
// and the full typographic scale once; each HUD then assigns its own panel/button geometry through
// the Scaled()/ScaledInt() helpers so no concrete draw routine carries a raw scaling constant.
struct PanelMetrics {
    float scale = 1.0f;

    // Shared geometry (scaled from the bases above).
    float margin = 0.0f;
    float lineH = 0.0f;
    float headerH = 0.0f;

    // Panel-specific geometry — assigned by each HUD via Scaled(); zero until set.
    float panelW = 0.0f;
    float panelH = 0.0f;
    float btnW = 0.0f;
    float btnH = 0.0f;
    float btnGap = 0.0f;
    float btnSpacing = 0.0f;

    // The full semantic typographic scale, resolved once for this DPI.
    int fontTitle = 0;
    int fontHeader = 0;
    int fontBody = 0;
    int fontLabel = 0;
    int fontSmall = 0;

    float Scaled(float base) const { return base * scale; }
    int   ScaledInt(float base) const { return static_cast<int>(base * scale); }

    // Resolve the shared margins and every font role for one scale factor. Panels call this in
    // Build(), then set their own panelW/btn* fields.
    static PanelMetrics Make(float scale) {
        PanelMetrics m;
        m.scale = scale;
        m.margin = kMarginBase * scale;
        m.lineH = kLineHBase * scale;
        m.headerH = kHeaderHBase * scale;
        m.fontTitle = FontSize(FontRole::Title, scale);
        m.fontHeader = FontSize(FontRole::Header, scale);
        m.fontBody = FontSize(FontRole::Body, scale);
        m.fontLabel = FontSize(FontRole::ButtonLabel, scale);
        m.fontSmall = FontSize(FontRole::Small, scale);
        return m;
    }
};

} // namespace Hud
