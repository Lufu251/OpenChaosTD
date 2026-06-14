#pragma once

#include <raylib.h>
#include <engine/core/text_renderer.hpp>
#include <engine/systems/ui_widgets.hpp> // kDefaultStyle
#include <hud/hud_theme.hpp>

// Shared chrome for the scrollable card-list screens (datapack select, map select, map editor
// catalog). Each state keeps its own per-card content (icon/thumbnail + text rows) and footer
// buttons; these helpers collapse the parts that were copy-pasted verbatim across all three: the
// card frame, the preview/"no preview" thumbnail, and the header/footer mask + title.

inline constexpr float kCardIconPad = 16.0f;  // inset of the icon/preview inside a card
inline constexpr float kCardThumbW  = 160.0f; // preview column width inside a card

// Card background + border, hover-styled.
inline void DrawCardFrame(Rectangle card, bool hovered) {
    DrawRectangleRec(card, hovered ? kDefaultStyle.m_bgHovered : kDefaultStyle.m_bgNormal);
    DrawRectangleLinesEx(card, hovered ? kDefaultStyle.m_borderWidthActive : kDefaultStyle.m_borderWidth,
                         hovered ? kDefaultStyle.m_borderSel : kDefaultStyle.m_border);
}

// Preview thumbnail fitted into `thumb`, or a bordered "no preview" placeholder when preview is null.
inline void DrawCardThumbnail(Rectangle thumb, const Texture2D* preview) {
    if (preview && preview->id != 0) {
        DrawRectangleRec(thumb, Hud::kBgDark);
        DrawTextureFitted(*preview, thumb);
        DrawRectangleLinesEx(thumb, 1.0f, kDefaultStyle.m_border);
    } else {
        DrawRectangleRec(thumb, Hud::kWorldBackground);
        DrawRectangleLinesEx(thumb, 1.0f, kDefaultStyle.m_border);
        DrawCenteredText("no preview", thumb.x + thumb.width / 2.0f,
                         thumb.y + thumb.height / 2.0f - 8.0f, 16, Hud::kTextSecondary);
    }
}

// Header mask + centered title and footer mask, covering any card that scrolled into those bands.
// Call after the cards + scrollbar; the caller then draws its own footer buttons over the footer mask.
inline void DrawListChrome(float screenW, float screenH, float listTop, float listBottom, const char* title) {
    DrawRectangle(0, 0, static_cast<int>(screenW), static_cast<int>(listTop), Hud::kWorldBackground);
    DrawCenteredText(title, screenW / 2.0f, 40.0f, static_cast<int>(Hud::kFontStateTitle), Hud::kTextPrimary);
    DrawRectangle(0, static_cast<int>(listBottom), static_cast<int>(screenW),
                  static_cast<int>(screenH - listBottom), Hud::kWorldBackground);
}

// Thumbnail rectangle inset within a card, matching the shared card layout.
inline Rectangle CardThumbRect(Rectangle card) {
    return {card.x + kCardIconPad, card.y + kCardIconPad, kCardThumbW, card.height - 2.0f * kCardIconPad};
}

// Text column geometry to the right of the thumbnail.
struct CardTextArea { float x, right, width; };
inline CardTextArea CardTextColumn(Rectangle card, Rectangle thumb, float gap = 20.0f) {
    CardTextArea a;
    a.x = thumb.x + thumb.width + gap;
    a.right = card.x + card.width - gap;
    a.width = a.right - a.x;
    return a;
}

// Center a panel of size (pw, ph) on a screen of size (sw, sh).
inline Rectangle CenteredPanel(float screenW, float screenH, float panelW, float panelH) {
    return {(screenW - panelW) / 2.0f, (screenH - panelH) / 2.0f, panelW, panelH};
}
