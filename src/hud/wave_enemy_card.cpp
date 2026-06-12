#include <hud/wave_enemy_card.hpp>
#include <hud/hud_draw.hpp>
#include <hud/hud_theme.hpp>
#include <engine/core/text_renderer.hpp>
#include <engine/core/resources.hpp>
#include <raylib.h>
#include <algorithm>

// Resolve a texture by key and aspect-fit it into `dest`. No-op if the key is missing.
static void DrawSpriteFit(const std::string& textureKey, Resources& assets, Rectangle dest) {
    if (!assets.HasTexture(textureKey)) return; // GetTexture throws on a missing key
    DrawTextureFitted(assets.GetTexture(textureKey), dest);
}

void WaveEnemyCard::SetContent(const WaveEnemyEntry& entry, const CardMetrics& metrics) {
    m_metrics    = metrics;
    m_count      = entry.m_count;
    m_name       = entry.m_name;
    m_level      = entry.m_level;
    m_hasProto   = entry.m_hasProto;
    m_textureKey = entry.m_textureKey;
    m_stats      = entry.m_stats;
}

float WaveEnemyCard::Measure() const {
    // Header row plus one row per module stat; never shorter than the sprite tile.
    float textH = static_cast<float>(1 + m_stats.size()) * m_metrics.lineH;
    float contentH = std::max(m_metrics.iconSize, textH);
    return contentH + 2.0f * m_metrics.pad;
}

void WaveEnemyCard::Draw(Rectangle bounds, Resources& assets) const {
    const float pad = m_metrics.pad;

    // Card frame: subtle fill plus a border so each enemy type reads as a distinct tile.
    Hud::DrawFramedBox(bounds, Hud::kCardFill, Hud::kCardBorder);

    // Icon tile on the left; the sprite draws directly over the card with a transparent backing.
    Rectangle icon = { bounds.x + pad, bounds.y + pad, m_metrics.iconSize, m_metrics.iconSize };
    if (m_hasProto)
        DrawSpriteFit(m_textureKey, assets, icon);

    // Text column to the right of the icon.
    float tx = icon.x + m_metrics.iconSize + pad;
    float ty = bounds.y + pad;
    float textRight = bounds.x + bounds.width - pad;

    // Header row: "Nx Name" on the left, "Lv N" badge right-aligned.
    Text::Draw(TextFormat("%dx %s", m_count, m_name.c_str()),
             static_cast<int>(tx), static_cast<int>(ty), m_metrics.fontSm, RAYWHITE);
    if (m_hasProto)
        Hud::DrawTextRightAligned(TextFormat("Lv %d", m_level), textRight, ty, m_metrics.fontSm, GOLD);
    ty += m_metrics.lineH;

    // One stat row per module, colored by the module (RAYWHITE core, Shield/Immune accents).
    Hud::DrawDescLines(m_stats, tx, ty, m_metrics.lineH, m_metrics.fontSm);
}
