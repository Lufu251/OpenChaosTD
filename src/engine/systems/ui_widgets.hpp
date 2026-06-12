#pragma once
#include <raylib.h>
#include <string>

struct WidgetStyle {
    Color m_bgNormal; // default background
    Color m_bgHovered; // button hover
    Color m_bgInput; // text input background
    Color m_bgActive; // toggle on state
    Color m_border; // normal border
    Color m_borderSel; // button selected border
    Color m_accent; // slider fill, focused input border
    Color m_text; // toggle label, input text
    float m_borderWidth;
    float m_borderWidthActive; // selected button, focused input
};

// Default palette for widgets.
inline const WidgetStyle kDefaultStyle{
    .m_bgNormal   = {40,  40,  40,  255},
    .m_bgHovered  = {65,  65,  65,  255},
    .m_bgInput    = {30,  30,  30,  255},
    .m_bgActive   = {80,  180, 80,  255},
    .m_border     = {80,  80,  80,  255},
    .m_borderSel  = {255, 180, 0,   255},
    .m_accent     = {100, 149, 237, 255},
    .m_text       = {255, 255, 255, 255},
    .m_borderWidth       = 1.0f,
    .m_borderWidthActive = 2.0f,
};

// Muted palette for non-interactive / unavailable widgets.
inline const WidgetStyle kDisabledStyle {
    .m_bgNormal   = {30,  30,  30,  200},
    .m_bgHovered  = {30,  30,  30,  200},
    .m_bgInput    = {20,  20,  20,  200},
    .m_bgActive   = {40,  40,  40,  200},
    .m_border     = {60,  60,  60,  255},
    .m_borderSel  = {60,  60,  60,  255},
    .m_accent     = {60,  60,  60,  255},
    .m_text       = {100, 100, 100, 255},
    .m_borderWidth       = 1.0f,
    .m_borderWidthActive = 1.0f,
};

struct Button {
    Rectangle m_rect = {};
    std::string m_label;

    void Update(Vector2 mouse, bool pressed);
    bool IsClicked() const { return m_clicked; }
    bool IsHovered() const { return m_hovered; }
    void Draw(bool selected = false, const WidgetStyle& style = kDefaultStyle) const;
    void DrawLabel(int fontSize, Color color) const;

private:
    bool m_hovered = false;
    bool m_clicked = false;
};

struct Slider {
    Rectangle m_rect = {};
    float m_value = 0.0f;
    float m_min = 0.0f;
    float m_max = 1.0f;
    float m_step = 0.0f; // 0 = continuous; otherwise snap to multiples of m_step from m_min

    void Update(Vector2 mouse, bool held);
    bool IsDragging() const { return m_dragging; }
    void Draw(const WidgetStyle& style = kDefaultStyle) const;

private:
    bool m_dragging = false;
};

struct Toggle {
    Rectangle m_rect = {};
    std::string m_label;
    bool m_value = false;

    void Update(Vector2 mouse, bool pressed);
    bool IsClicked() const { return m_clicked; }
    void Draw(const WidgetStyle& style = kDefaultStyle) const;

private:
    bool m_clicked = false;
};

struct TextInput {
    Rectangle m_rect = {};
    std::string m_text;
    int m_maxLength = 64;

    void Update(Vector2 mouse, bool pressed);
    bool IsFocused() const { return m_focused; }
    void Draw(const WidgetStyle& style = kDefaultStyle) const;

private:
    bool m_focused = false;
};

struct ProgressBar {
    Rectangle m_rect = {};
    float m_value = 0.0f;
    float m_max = 1.0f;

    void Draw(const WidgetStyle& style = kDefaultStyle) const;
};

// Geometry of a ScrollableList in raw virtual coords. Defaults match the picker
// screens; pass a custom config to tune card size or band insets.
struct ScrollableListConfig {
    float m_margin = 40.0f;      // left/right inset of cards; gutter for the scrollbar
    float m_listTop = 110.0f;    // top of the scrolling band (below the title)
    float m_footerH = 80.0f;     // bottom strip reserved for a back button
    float m_cardH = 120.0f;      // fixed card height
    float m_cardGap = 12.0f;     // vertical gap between cards
    float m_scrollSpeed = 40.0f; // virtual px panned per wheel notch
};

// Vertically scrolling list of fixed-height cards inside a header/footer-masked band.
// Owns the scroll offset and hovered index plus all geometry; the caller renders each
// card's contents into CardRect(i) and masks the overflow above/below the band itself.
class ScrollableList {
public:
    ScrollableList() = default;
    explicit ScrollableList(const ScrollableListConfig& cfg) : m_cfg(cfg) {}

    void Reset() { m_scroll = 0.0f; m_hovered = -1; }

    float ListTop() const { return m_cfg.m_listTop; }
    float ListBottom(float screenH) const { return screenH - m_cfg.m_footerH; }
    float MaxScroll(int count, float screenH) const;
    // On-screen rect of card `index`, accounting for the current scroll offset.
    Rectangle CardRect(int index, float screenW, float screenH) const;

    // Mouse-wheel pans the list, clamped to the content extent.
    void ProcessScroll(float wheel, int count, float screenH);
    // Refresh the hovered card and return the index clicked this frame, or -1. Only cards
    // inside the visible band are considered, so clicks on masked overflow are ignored.
    int ProcessHover(Vector2 mouse, bool clicked, int count, float screenW, float screenH);
    int Hovered() const { return m_hovered; }

    // Draw the scrollbar track + thumb; no-op when the content fits the band.
    void DrawScrollbar(int count, float screenW, float screenH,
                       Color trackColor, Color thumbColor) const;

private:
    ScrollableListConfig m_cfg;
    float m_scroll = 0.0f;
    int m_hovered = -1; // index of the card under the cursor, or -1
};
