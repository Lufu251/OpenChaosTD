#include <hud/hud_theme.hpp>
#include <engine/util/file_store.hpp>
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

} // namespace

void Hud::LoadConfig(FileStore& fileStore) {
    if (!fileStore.Exists("config/hud.toml")) return;
    const toml::table tbl = fileStore.LoadToml("config/hud.toml");

    // Palette
    if (const toml::table* pal = tbl["palette"].as_table()) {
        if (auto a = (*pal)["panelBorder"].as_array())      kPanelBorder       = ParseColor(*a);
        if (auto a = (*pal)["textMuted"].as_array())        kTextMuted         = ParseColor(*a);
        if (auto a = (*pal)["upgradeReady"].as_array())     kUpgradeReady      = ParseColor(*a);
        if (auto a = (*pal)["tooltipBorder"].as_array())    kTooltipBorder     = ParseColor(*a);
        if (auto a = (*pal)["cardFill"].as_array())         kCardFill          = ParseColor(*a);
        if (auto a = (*pal)["cardBorder"].as_array())       kCardBorder        = ParseColor(*a);
        if (auto a = (*pal)["infinityGlyph"].as_array())    kInfinityGlyph     = ParseColor(*a);
        if (auto a = (*pal)["highlight"].as_array())        kHighlight         = ParseColor(*a);
        if (auto a = (*pal)["costAffordable"].as_array())   kCostAffordable    = ParseColor(*a);
        if (auto a = (*pal)["costUnaffordable"].as_array()) kCostUnaffordable  = ParseColor(*a);
        if (auto a = (*pal)["sellLabel"].as_array())        kSellLabel         = ParseColor(*a);
        if (auto a = (*pal)["targetLabel"].as_array())      kTargetLabel       = ParseColor(*a);
        if (auto a = (*pal)["panelBgRgb"].as_array())       kPanelBgRgb        = ParseColor(*a);
        if (auto a = (*pal)["eventTextRgb"].as_array())     kEventTextRgb      = ParseColor(*a);
        if (auto a = (*pal)["screenDim"].as_array())        kScreenDim         = ParseColor(*a);
    }

    // Alpha values
    if (const toml::table* al = tbl["alpha"].as_table()) {
        if (auto v = (*al)["window"].value<int>())      kPanelAlphaWindow = static_cast<unsigned char>(*v);
        if (auto v = (*al)["docked"].value<int>())      kPanelAlphaDocked = static_cast<unsigned char>(*v);
        if (auto v = (*al)["overlayBg"].value<int>())   kOverlayBgAlpha   = static_cast<unsigned char>(*v);
        if (auto v = (*al)["overlayText"].value<int>()) kOverlayTextAlpha = static_cast<unsigned char>(*v);
        if (auto v = (*al)["tooltipBg"].value<int>())   kTooltipBgAlpha   = static_cast<unsigned char>(*v);
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
        if (const toml::table* c = (*su)["colors"].as_table()) {
            if (auto a = (*c)["background"].as_array())    kStateBackground = ParseColor(*a);
            if (auto a = (*c)["dialogBg"].as_array())      kDialogBg        = ParseColor(*a);
            if (auto a = (*c)["dialogOverlay"].as_array()) kDialogOverlay   = ParseColor(*a);
            if (auto a = (*c)["disabledText"].as_array())  kDisabledText    = ParseColor(*a);
            if (auto a = (*c)["warning"].as_array())       kWarning         = ParseColor(*a);
            if (auto a = (*c)["placeholderBg"].as_array()) kPlaceholderBg   = ParseColor(*a);
            if (auto a = (*c)["subtle"].as_array())        kSubtle          = ParseColor(*a);
            if (auto a = (*c)["victory"].as_array())       kVictory         = ParseColor(*a);
            if (auto a = (*c)["defeat"].as_array())        kDefeat          = ParseColor(*a);
        }
    }

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
