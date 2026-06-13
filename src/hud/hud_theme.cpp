#include <hud/hud_theme.hpp>
#include <engine/util/file_store.hpp>
#include <engine/systems/ui_widgets.hpp>
#include <toml++/toml.hpp>

namespace {

Color ParseColor(const toml::array& a) {
    return {
        static_cast<unsigned char>(a[0].value_or(0)),
        static_cast<unsigned char>(a[1].value_or(0)),
        static_cast<unsigned char>(a[2].value_or(0)),
        static_cast<unsigned char>(a.size() >= 4 ? a[3].value_or(255) : 255)
    };
}

// Mix a color toward white (lighten) or black (darken); WithAlpha overrides opacity only.
// These let the widget surfaces be derived from the palette instead of hand-picked, so the
// recessed widget chrome tracks any palette automatically (see ApplyWidgetStyles).
Color Lighten(Color c, float amt)            { return ColorLerp(c, WHITE, amt); }
Color Darken(Color c, float amt)             { return ColorLerp(c, BLACK, amt); }
Color WithAlpha(Color c, unsigned char a)    { c.a = a; return c; }

// Derive the global widget styles from the palette so they can never drift out of alignment.
// Interactive accents map straight to palette roles; the recessed widget surfaces (bgNormal /
// bgHovered / bgInput and the disabled set) are not palette roles of their own, so they are
// derived from the panel fill (kPanelBgRgb) rather than hand-picked — change the palette and the
// widget chrome follows. Called unconditionally at load, so widgets follow the palette whether or
// not config/hud.toml exists; m_borderWidth / m_borderWidthActive keep the engine defaults.
void ApplyWidgetStyles() {
    using namespace Hud;

    // Default (interactive) widget surfaces — a clear step above the panel fill so buttons,
    // sliders and cards lift off the chrome, brightening further on hover; inputs sit recessed.
    kDefaultStyle.m_bgNormal  = Lighten(kPanelBgRgb, 0.06f);
    kDefaultStyle.m_bgHovered = Lighten(kPanelBgRgb, 0.15f);
    kDefaultStyle.m_bgInput   = Darken(kPanelBgRgb, 0.19f);
    kDefaultStyle.m_bgActive  = kStatusPositive; // toggle-on fill
    kDefaultStyle.m_border    = kPanelBorder;
    kDefaultStyle.m_borderSel = kHighlight;      // selection border
    kDefaultStyle.m_accent    = kAccent;         // slider fill, focused input border
    kDefaultStyle.m_text      = kTextPrimary;

    // Disabled widget surfaces: dimmed panel-fill tints at a uniform low alpha, with chrome
    // sitting halfway between the panel fill and its border so it reads muted but present.
    constexpr unsigned char kDisabledAlpha = 200;
    const Color disabledChrome = ColorLerp(kPanelBgRgb, kPanelBorder, 0.5f);

    kDisabledStyle.m_bgNormal  = WithAlpha(Darken(kPanelBgRgb, 0.08f), kDisabledAlpha);
    kDisabledStyle.m_bgHovered = kDisabledStyle.m_bgNormal;
    kDisabledStyle.m_bgInput   = WithAlpha(Darken(kPanelBgRgb, 0.33f), kDisabledAlpha);
    kDisabledStyle.m_bgActive  = WithAlpha(Lighten(kPanelBgRgb, 0.03f), kDisabledAlpha);
    kDisabledStyle.m_border    = disabledChrome;
    kDisabledStyle.m_borderSel = disabledChrome;
    kDisabledStyle.m_accent    = disabledChrome;
    kDisabledStyle.m_text      = kTextDisabled;
}

} // namespace

void Hud::LoadConfig(FileStore& fileStore) {
    if (!fileStore.Exists("config/hud.toml")) {
        // No file: palette keeps its header defaults — still derive the widget styles from them.
        ApplyWidgetStyles();
        return;
    }
    const toml::table tbl = fileStore.LoadToml("config/hud.toml");

    // Palette: text hierarchy, then functional status accents, then surfaces and chrome.
    if (const toml::table* pal = tbl["palette"].as_table()) {
        if (auto a = (*pal)["textHeader"].as_array()) kTextHeader = ParseColor(*a);
        if (auto a = (*pal)["textPrimary"].as_array()) kTextPrimary = ParseColor(*a);
        if (auto a = (*pal)["textSecondary"].as_array()) kTextSecondary = ParseColor(*a);
        if (auto a = (*pal)["textDisabled"].as_array()) kTextDisabled = ParseColor(*a);
        if (auto a = (*pal)["statusPositive"].as_array()) kStatusPositive = ParseColor(*a);
        if (auto a = (*pal)["statusNegative"].as_array()) kStatusNegative = ParseColor(*a);
        if (auto a = (*pal)["highlight"].as_array()) kHighlight = ParseColor(*a);
        if (auto a = (*pal)["accent"].as_array()) kAccent = ParseColor(*a);
        if (auto a = (*pal)["panelBorder"].as_array()) kPanelBorder = ParseColor(*a);
        if (auto a = (*pal)["worldBg"].as_array()) kWorldBackground = ParseColor(*a);
        if (auto a = (*pal)["bgDark"].as_array()) kBgDark = ParseColor(*a);
        if (auto a = (*pal)["screenDim"].as_array()) kScreenDim = ParseColor(*a);
        if (auto a = (*pal)["iconTint"].as_array()) kIconTint = ParseColor(*a);
        if (auto a = (*pal)["panelBgRgb"].as_array()) kPanelBgRgb = ParseColor(*a);
    }

    // Alpha values
    if (const toml::table* al = tbl["alpha"].as_table()) {
        if (auto v = (*al)["window"].value<int>())      kPanelAlphaWindow = static_cast<unsigned char>(*v);
        if (auto v = (*al)["docked"].value<int>())      kPanelAlphaDocked = static_cast<unsigned char>(*v);
        if (auto v = (*al)["overlayBg"].value<int>())   kOverlayBgAlpha   = static_cast<unsigned char>(*v);
        if (auto v = (*al)["overlayText"].value<int>()) kOverlayTextAlpha = static_cast<unsigned char>(*v);
        if (auto v = (*al)["tooltipBg"].value<int>())   kTooltipBgAlpha   = static_cast<unsigned char>(*v);
        if (auto v = (*al)["dialog"].value<int>())      kDialogAlpha      = static_cast<unsigned char>(*v);
    }

    // Typographic scale
    if (const toml::table* ty = tbl["typography"].as_table()) {
        if (auto v = (*ty)["title"].value<float>())       kFontTitleBase       = *v;
        if (auto v = (*ty)["header"].value<float>())      kFontHeaderBase      = *v;
        if (auto v = (*ty)["body"].value<float>())        kFontBodyBase        = *v;
        if (auto v = (*ty)["buttonLabel"].value<float>()) kFontButtonLabelBase = *v;
        if (auto v = (*ty)["small"].value<float>())       kFontSmallBase       = *v;
    }

    // Full-screen state UI typography and chrome
    if (const toml::table* su = tbl["state_ui"].as_table()) {
        if (auto v = (*su)["title"].value<float>())       kFontStateTitle  = *v;
        if (auto v = (*su)["screenTitle"].value<float>()) kFontScreenTitle = *v;
        if (auto v = (*su)["menuButton"].value<float>())  kFontMenuButton  = *v;

        // State-specific cosmetics, one sub-section per owning screen.
        if (const toml::table* sel = (*su)["select"].as_table()) {
            if (auto a = (*sel)["autoCardTint"].as_array()) g_selectTheme.autoCardTint = ParseColor(*a);
        }
        // Only the canvas/export backdrops remain state-specific; grid, outline and brush ghosts
        // derive from palette roles in map_editor_state.cpp.
        if (const toml::table* me = (*su)["map_editor"].as_table()) {
            if (auto a = (*me)["canvasBg"].as_array()) g_mapEditorTheme.canvasBg = ParseColor(*a);
            if (auto a = (*me)["exportBg"].as_array()) g_mapEditorTheme.exportBg = ParseColor(*a);
        }
    }

    // Derive the global widget styles from the (now loaded) palette.
    ApplyWidgetStyles();

    // Shared layout anchors
    if (const toml::table* la = tbl["layout"].as_table()) {
        if (auto v = (*la)["statusBarBaseH"].value<float>()) kStatusBarBaseHeight = *v;
        if (auto v = (*la)["wavePanelGap"].value<float>())   kWavePanelGap        = *v;
        if (auto v = (*la)["marginBase"].value<float>())     kMarginBase          = *v;
        if (auto v = (*la)["lineHBase"].value<float>())      kLineHBase           = *v;
        if (auto v = (*la)["headerHBase"].value<float>())    kHeaderHBase         = *v;
    }

    // Event toast limits
    if (const toml::table* ev = tbl["events"].as_table()) {
        if (auto v = (*ev)["maxEntries"].value<int>())   g_eventCfg.maxEntries = *v;
        if (auto v = (*ev)["fadeTime"].value<float>())   g_eventCfg.fadeTime   = *v;
    }

    // Per-panel dimensions — one flat [panels] table, each key prefixed by its panel. Every read
    // falls back to the struct default, so a partial table still loads.
    if (const toml::table* p = tbl["panels"].as_table()) {
        if (auto v = (*p)["pauseWidth"].value<float>())          g_pauseCfg.width      = *v;
        if (auto v = (*p)["pauseHeight"].value<float>())         g_pauseCfg.height     = *v;
        if (auto v = (*p)["pauseButtonWidth"].value<float>())    g_pauseCfg.btnW       = *v;

        if (auto v = (*p)["statusWaveButtonWidth"].value<float>())   g_statusCfg.waveW  = *v;
        if (auto v = (*p)["statusAutoButtonWidth"].value<float>())   g_statusCfg.autoW  = *v;
        if (auto v = (*p)["statusWavesButtonWidth"].value<float>())  g_statusCfg.wavesW = *v;
        if (auto v = (*p)["statusMargin"].value<float>())            g_statusCfg.margin = *v;

        if (auto v = (*p)["waveWidth"].value<float>())       g_waveCfg.width       = *v;
        if (auto v = (*p)["waveCardGap"].value<float>())     g_waveCfg.cardGap     = *v;
        if (auto v = (*p)["waveCardPadding"].value<float>()) g_waveCfg.cardPadding = *v;
        if (auto v = (*p)["waveIconSize"].value<float>())    g_waveCfg.iconSize    = *v;

        if (auto v = (*p)["buildButtonSize"].value<float>())  g_buildCfg.btnSize = *v;
        if (auto v = (*p)["buildPanelHeight"].value<float>()) g_buildCfg.panelH  = *v;
        if (auto v = (*p)["buildGap"].value<float>())         g_buildCfg.gap     = *v;

        if (auto v = (*p)["infoWidth"].value<float>())     g_infoCfg.width     = *v;
        if (auto v = (*p)["infoAnchorGap"].value<float>()) g_infoCfg.anchorGap = *v;
    }
}
