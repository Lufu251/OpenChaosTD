#include <hud/hud_draw.hpp>
#include <hud/hud_theme.hpp>

namespace Hud {

void DrawFramedBox(Rectangle rect, Color fill, Color border) {
    DrawRectangleRec(rect, fill);
    DrawRectangleLinesEx(rect, 1.0f, border);
}

void DrawTextRightAligned(const char* text, float rightEdge, float y, int fontSize, Color color,
                          Text::Kind kind) {
    int w = Text::Measure(text, fontSize, kind);
    Text::Draw(text, static_cast<int>(rightEdge) - w, static_cast<int>(y), fontSize, color, kind);
}

float DrawDescLines(const std::vector<DescLine>& lines, float x, float y, float lineH, int fontSize) {
    for (const auto& line : lines) {
        Text::Draw(line.m_text.c_str(), static_cast<int>(x), static_cast<int>(y), fontSize, line.m_color);
        y += lineH;
    }
    return y;
}

void DrawToggleableButton(const Button& btn, bool enabled, int fontSize, Color enabledColor) {
    btn.Draw(false, enabled ? kDefaultStyle : kDisabledStyle);
    btn.DrawLabel(fontSize, enabled ? enabledColor : DARKGRAY);
}

void DrawHighlightButton(const Button& btn, bool highlighted, int fontSize,
                         Color activeColor, Color normalColor) {
    btn.Draw(highlighted); // selected => gold border from kDefaultStyle
    btn.DrawLabel(fontSize, highlighted ? activeColor : normalColor);
}

void DrawOverlayToast(const char* text, Rectangle bg, float textX, float textY, int fontSize,
                      float fade) {
    auto faded = [fade](unsigned char a) { return static_cast<unsigned char>(a * fade); };
    DrawRectangleRec(bg, PanelBg(faded(kOverlayBgAlpha)));
    Text::Draw(text, static_cast<int>(textX), static_cast<int>(textY), fontSize,
               EventText(faded(kOverlayTextAlpha)));
}

} // namespace Hud
