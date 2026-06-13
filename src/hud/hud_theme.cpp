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

// Derive the global widget styles from the palette so they can never drift out of alignment.
// Interactive accents map straight to palette roles; the recessed widget surfaces (bgNormal /
// bgHovered / bgInput and the disabled greys) are not palette roles — they belong to the widget
// chrome alone — so they are named here rather than left to the engine's generic-grey defaults.
// Called unconditionally at load, so widgets follow the palette whether or not config/hud.toml
// exists; m_borderWidth / m_borderWidthActive keep the engine defaults (config never set them).
void ApplyWidgetStyles() {
    using namespace Hud;

    // Default (interactive) widget surfaces.
    constexpr Color kWidgetBg{33, 38, 52, 255};
    constexpr Color kWidgetBgHover{47, 54, 72, 255};
    constexpr Color kWidgetInput{22, 26, 36, 255};

    kDefaultStyle.m_bgNormal  = kWidgetBg;
    kDefaultStyle.m_bgHovered = kWidgetBgHover;
    kDefaultStyle.m_bgInput   = kWidgetInput;
    kDefaultStyle.m_bgActive  = kStatusPositive; // toggle-on fill
    kDefaultStyle.m_border    = kPanelBorder;
    kDefaultStyle.m_borderSel = kHighlight;      // selection border
    kDefaultStyle.m_accent    = kAccent;         // slider fill, focused input border
    kDefaultStyle.m_text      = kTextPrimary;

    // Disabled widget surfaces: muted slate, all at the same low alpha.
    constexpr Color kDisabledBg{24, 27, 36, 200};
    constexpr Color kDisabledInput{18, 21, 29, 200};
    constexpr Color kDisabledActive{30, 34, 46, 200};
    constexpr Color kDisabledChrome{44, 50, 66, 255}; // border, selection border and accent

    kDisabledStyle.m_bgNormal  = kDisabledBg;
    kDisabledStyle.m_bgHovered = kDisabledBg;
    kDisabledStyle.m_bgInput   = kDisabledInput;
    kDisabledStyle.m_bgActive  = kDisabledActive;
    kDisabledStyle.m_border    = kDisabledChrome;
    kDisabledStyle.m_borderSel = kDisabledChrome;
    kDisabledStyle.m_accent    = kDisabledChrome;
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
        if (const toml::table* me = (*su)["map_editor"].as_table()) {
            if (auto a = (*me)["canvasBg"].as_array()) g_mapEditorTheme.canvasBg = ParseColor(*a);
            if (auto a = (*me)["exportBg"].as_array()) g_mapEditorTheme.exportBg = ParseColor(*a);
            if (auto a = (*me)["grid"].as_array()) g_mapEditorTheme.grid = ParseColor(*a);
            if (auto a = (*me)["brushOutline"].as_array()) g_mapEditorTheme.brushOutline = ParseColor(*a);
            if (auto a = (*me)["brushGrass"].as_array()) g_mapEditorTheme.brushGrass = ParseColor(*a);
            if (auto a = (*me)["brushRock"].as_array()) g_mapEditorTheme.brushRock = ParseColor(*a);
            if (auto a = (*me)["brushCore"].as_array()) g_mapEditorTheme.brushCore = ParseColor(*a);
            if (auto a = (*me)["brushNest"].as_array()) g_mapEditorTheme.brushNest = ParseColor(*a);
            if (auto a = (*me)["brushBuff"].as_array()) g_mapEditorTheme.brushBuff = ParseColor(*a);
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

    // Per-panel dimensions
    if (const toml::table* panels = tbl["panels"].as_table()) {
        if (const toml::table* p = (*panels)["pause"].as_table()) {
            if (auto v = (*p)["width"].value<float>())         g_pauseCfg.width      = *v;
            if (auto v = (*p)["height"].value<float>())        g_pauseCfg.height     = *v;
            if (auto v = (*p)["buttonWidth"].value<float>())   g_pauseCfg.btnW       = *v;
        }
        if (const toml::table* s = (*panels)["status"].as_table()) {
            if (auto v = (*s)["waveButtonWidth"].value<float>())   g_statusCfg.waveW  = *v;
            if (auto v = (*s)["autoButtonWidth"].value<float>())   g_statusCfg.autoW  = *v;
            if (auto v = (*s)["wavesButtonWidth"].value<float>())  g_statusCfg.wavesW = *v;
            if (auto v = (*s)["margin"].value<float>())            g_statusCfg.margin = *v;
        }
        if (const toml::table* w = (*panels)["wave"].as_table()) {
            if (auto v = (*w)["width"].value<float>())       g_waveCfg.width       = *v;
            if (auto v = (*w)["cardGap"].value<float>())     g_waveCfg.cardGap     = *v;
            if (auto v = (*w)["cardPadding"].value<float>()) g_waveCfg.cardPadding = *v;
            if (auto v = (*w)["iconSize"].value<float>())    g_waveCfg.iconSize    = *v;
        }
        if (const toml::table* b = (*panels)["tower_build"].as_table()) {
            if (auto v = (*b)["buttonSize"].value<float>())  g_buildCfg.btnSize = *v;
            if (auto v = (*b)["panelHeight"].value<float>()) g_buildCfg.panelH  = *v;
            if (auto v = (*b)["gap"].value<float>())         g_buildCfg.gap     = *v;
        }
        if (const toml::table* i = (*panels)["tower_info"].as_table()) {
            if (auto v = (*i)["width"].value<float>())       g_infoCfg.width     = *v;
            if (auto v = (*i)["anchorGap"].value<float>())   g_infoCfg.anchorGap = *v;
        }
    }
}
