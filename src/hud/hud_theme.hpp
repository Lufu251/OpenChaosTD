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
// One unified family of semantic roles, shared by every HUD panel AND every full-screen state:
// deep blue-slate surfaces, warm amber for headers and focus, mint/rose for positive/negative
// status, sky cyan for informational accents. Every color drawn anywhere is a role named here —
// concrete draw methods never reach for raw raylib color literals.

// Text hierarchy: every string a panel or screen renders picks exactly one of these roles.
inline Color kTextHeader{228, 214, 160, 255}; // panel/screen headers, titles, category sub-headers, event toasts
inline Color kTextPrimary{233, 238, 246, 255}; // primary readouts, button labels, card names, dialog titles
inline Color kTextSecondary{162, 174, 196, 255}; // secondary labels: descriptions, budgets, hints, subtitles
// Disabled label text. Text-specific on purpose: a disabled widget's *background* comes from the
// widget style (kDisabledStyle), but its label always reads through this key so disabled text
// stays equally legible on every panel.
inline Color kTextDisabled{100, 110, 132, 255};

// Functional status accents, shared by every affordance of the same polarity. Action-oriented roles
// carry dedicated high-saturation hues so affordability, warnings and focus read at a glance.
inline Color kStatusPositive{84, 200, 120, 255}; // affordable cost, upgrade ready, sell value
inline Color kStatusNegative{230, 80, 84, 255}; // unaffordable cost, warn/fail states, defeat title, delete action
inline Color kHighlight{255, 194, 64, 255}; // focus: active toggles, tooltip/dialog border, badges, warnings, victory title
// Informational accent shared across HUD and screens: retarget label, settings binding-group
// headers, key-capture indicator, status-bar endless-mode infinity glyph.
inline Color kAccent{82, 168, 240, 255};

// Surfaces and chrome. Surfaces step deep→up for contrast; panels sit a clear step above the
// backdrop, framed by a bright dedicated border.
inline Color kPanelBorder{92, 106, 138, 255}; // panel / card / dialog borders and widget outlines
inline Color kWorldBackground{22, 26, 38, 255}; // gameplay clear behind the map, full-screen state clear, placeholder fills
inline Color kBgDark{12, 14, 22, 255}; // high-contrast dark backing: thumbnails, preview frames, swatch backs
inline Color kScreenDim{8, 10, 16, 165}; // full-screen dim behind the pause menu and modal dialogs
// Sprite tint for icon draws (build-bar towers, datapack icons); white = untinted.
inline Color kIconTint{255, 255, 255, 255};

// RGB base for the dynamic-alpha fill helper; alpha is ignored here and supplied per call.
inline Color kPanelBgRgb{34, 40, 56, 255};

// The dark panel fill is shared but drawn at varying opacity (window/docked/tooltip/card/dialog),
// so it is a helper rather than a constant. The event toast text fades too and reads through the
// header amber, so its RGB lives behind a helper keyed on kTextHeader.
inline Color PanelBg(unsigned char alpha)   { return {kPanelBgRgb.r, kPanelBgRgb.g, kPanelBgRgb.b, alpha}; }
inline Color EventText(unsigned char alpha) { return {kTextHeader.r, kTextHeader.g, kTextHeader.b, alpha}; }

// --- Panel-class background standards ---------------------------------------
// Every panel falls into one of three semantic classes; each class has one opacity (and border
// rule), so panels never pick an ad-hoc alpha. See HUD::DrawWindowBackground / DrawDockedBackground.
inline unsigned char kPanelAlphaWindow = 220; // Primary windows: Pause, TowerInfo, Wave (+ border)
inline unsigned char kPanelAlphaDocked = 200; // Docked bars: Status, Build (no border)
inline unsigned char kOverlayBgAlpha   = 160; // Ephemeral toast background, before the fade factor
inline unsigned char kOverlayTextAlpha = 220; // Ephemeral toast text, before the fade factor
inline unsigned char kTooltipBgAlpha   = 235; // Upgrade tooltip fill
inline unsigned char kDialogAlpha      = 245; // Modal dialog panel fill (PanelBg(kDialogAlpha))

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

// Full-screen states draw text, surfaces and status through the shared palette above: titles and
// menu labels use kTextPrimary, section headers kTextHeader, subtitles kTextSecondary, greyed
// labels kTextDisabled; the screen clear and placeholder fills use kWorldBackground; warnings and
// the victory title use kHighlight, the defeat title kStatusNegative; the modal panel fill is
// PanelBg(kDialogAlpha) and both the pause and modal dims use kScreenDim.

// --- State-specific cosmetics -------------------------------------------------
// Highly specialized colors owned by a single screen, grouped per state so config/hud.toml
// mirrors ownership. LoadConfig() overwrites these from the [state_ui.*] sub-sections; the
// defaults match the file so its removal is safe. Tints that need transparency carry their
// alpha here (and as the fourth TOML component) so opacity stays configurable.

// Selection screens (map / map-editor catalogs). Preview thumbnails use the shared kBgDark.
struct SelectTheme {
    Color autoCardTint{40, 50, 68, 255}; // procedural-map card's distinct thumbnail backing
};

// Map editor canvas. The grid lines, brush ghost tints and tile outline are derived in code from
// palette roles (see BrushTint / DrawEditCanvas in map_editor_state.cpp); only the two backdrops
// that are not palette roles — the canvas backing and the export clear — live here.
struct MapEditorTheme {
    Color canvasBg{20, 23, 32, 255}; // edit canvas backdrop
    Color exportBg{0, 0, 0, 255};    // offscreen clear behind the exported map.png
};

inline SelectTheme g_selectTheme;
inline MapEditorTheme g_mapEditorTheme;

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
// Unscaled base dimensions for each HUD panel. LoadConfig() overwrites these from the flat
// [panels] table in config/hud.toml; the defaults match the previous hardcoded values so removal
// of the file is safe.

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
