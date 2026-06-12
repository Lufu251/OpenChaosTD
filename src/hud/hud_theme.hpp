#pragma once

#include <raylib.h>

// Shared HUD theme: the single source of truth for panel colors, the semantic typographic scale,
// the panel-class background standards, the sibling-layout anchor (status-bar height), and the
// per-panel layout metrics that several HUDs would otherwise each re-declare and re-scale. Keeping
// these here means a palette, DPI, or typography change touches one file.
namespace Hud {

// --- Palette ----------------------------------------------------------------
// Panel chrome.
inline constexpr Color kPanelBorder  {80, 80, 80, 255};
// Secondary / dim body text (descriptions, budgets, placeholders).
inline constexpr Color kTextMuted    {180, 180, 180, 255};
// Upgrade button label when the purchase is affordable.
inline constexpr Color kUpgradeReady {160, 240, 120, 255};
// Upgrade tooltip border (gold accent).
inline constexpr Color kTooltipBorder{255, 180, 0, 255};
// Wave enemy card frame.
inline constexpr Color kCardFill     {40, 40, 48, 200};
inline constexpr Color kCardBorder   {90, 90, 100, 255};
// Status-bar endless-mode infinity glyph.
inline constexpr Color kInfinityGlyph{120, 180, 220, 255};

// Semantic accents — named so concrete draw methods never reach for raw raylib color literals.
inline constexpr Color kHighlight       {255, 203, 0,   255}; // active toggle (Speed/Auto) — GOLD
inline constexpr Color kCostAffordable  {0,   228, 48,  255}; // build cost the player can pay — GREEN
inline constexpr Color kCostUnaffordable{230, 41,  55,  255}; // build cost too expensive — RED
inline constexpr Color kSellLabel       {0,   228, 48,  255}; // sell button label — GREEN
inline constexpr Color kTargetLabel     {102, 191, 255, 255}; // retarget button label — SKYBLUE

// The dark panel fill is shared but drawn at varying opacity, so it is a helper rather than a
// constant. The event toast text likewise fades, so its RGB lives behind a helper too.
inline Color PanelBg(unsigned char alpha) { return {20, 20, 20, alpha}; }
inline Color EventText(unsigned char alpha) { return {255, 220, 80, alpha}; }

// --- Panel-class background standards ---------------------------------------
// Every panel falls into one of three semantic classes; each class has one opacity (and border
// rule), so panels never pick an ad-hoc alpha. See HUD::DrawWindowBackground / DrawDockedBackground.
inline constexpr unsigned char kPanelAlphaWindow = 220; // Primary windows: Pause, TowerInfo, Wave (+ border)
inline constexpr unsigned char kPanelAlphaDocked = 200; // Docked bars: Status, Build (no border)
inline constexpr unsigned char kOverlayBgAlpha   = 160; // Ephemeral toast background, before the fade factor
inline constexpr unsigned char kOverlayTextAlpha = 220; // Ephemeral toast text, before the fade factor
inline constexpr unsigned char kTooltipBgAlpha   = 235; // Upgrade tooltip fill
inline constexpr Color kScreenDim {0, 0, 0, 120};        // Full-screen dim behind the modal pause menu

// Event toast row geometry (unscaled): the background box hugs the measured text with this padding.
inline constexpr float kToastRowH    = 20.0f; // vertical stride between stacked toasts
inline constexpr float kToastPadX    = 2.0f;  // bg left extension and row-height shrink
inline constexpr float kToastPadTop  = 1.0f;  // bg top extension above the row
inline constexpr float kToastPadW    = 10.0f; // bg width beyond the text width
inline constexpr float kToastTextX   = 3.0f;  // text inset from the bar margin
inline constexpr float kToastTextY   = 2.0f;  // text inset from the row top

// --- Semantic typographic scale ---------------------------------------------
// Concrete HUDs request a role rather than a raw font-size integer; every panel that shows the same
// kind of text therefore renders at the same scaled size. Bases are unscaled; FontSize() applies DPI.
enum class FontRole { Title, Header, Body, ButtonLabel, Small };
inline constexpr float kFontTitleBase       = 28.0f; // modal title (PAUSED)
inline constexpr float kFontHeaderBase      = 14.0f; // panel headers, status readout
inline constexpr float kFontBodyBase        = 11.0f; // descriptions, stat rows, card text
inline constexpr float kFontButtonLabelBase = 12.0f; // button labels
inline constexpr float kFontSmallBase       = 8.0f;  // compact build-bar name/cost

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

// --- Sibling layout anchor --------------------------------------------------
// Unscaled height of the top StatusHUD bar. EventHUD and WaveHUD position their content below the
// bar; reading this (then applying their own Scaled()) keeps them in sync if the bar height changes.
inline constexpr float kStatusBarBaseHeight = 36.0f;
// Extra gap WaveHUD leaves below the status bar.
inline constexpr float kWavePanelGap = 6.0f;

// --- Shared panel metrics ---------------------------------------------------
// Unscaled bases shared by every text panel, so line height and margins no longer diverge per panel.
inline constexpr float kMarginBase  = 8.0f;
inline constexpr float kLineHBase   = 14.0f;
inline constexpr float kHeaderHBase = 20.0f;

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
