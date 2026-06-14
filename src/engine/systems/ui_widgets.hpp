#pragma once
#include <raylib.h>
#include <cstdint>
#include <string>
#include <vector>

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

// Default palette for widgets. Non-const on purpose: the host application may restyle these at
// startup (e.g. from a theme config); every widget drawn without an explicit style follows.
inline WidgetStyle kDefaultStyle{
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

// Muted palette for non-interactive / unavailable widgets. Restylable like kDefaultStyle.
inline WidgetStyle kDisabledStyle {
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
    bool m_enabled = true;          // when false, Update() is a no-op and Draw() uses kDisabledStyle
    int m_fontSize = 0;             // 0 = caller draws label manually via DrawLabel()
    Color m_labelColor = kDefaultStyle.m_text;

    void Update(Vector2 mouse, bool pressed);
    bool IsClicked() const { return m_clicked; }
    bool IsHovered() const { return m_hovered; }
    // Draw background, border, and — when m_fontSize != 0 and m_label is non-empty — the label
    // centred inside m_rect using m_fontSize and m_labelColor (muted when !m_enabled).
    void Draw(bool selected = false, const WidgetStyle& style = kDefaultStyle) const;
    // Manual label draw for callers that need custom positioning (e.g. build-bar icons).
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
    std::string m_placeholder; // drawn in muted color when m_text is empty and not focused
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

    // True when any part of card `index` is inside the visible band.
    bool IsCardVisible(int index, float screenH) const;

    // Centered "back" button rectangle in the footer band, sized 160×44 by default.
    Rectangle FooterButtonRect(float screenW, float screenH, float btnW = 160.0f,
                               float btnH = 44.0f) const;

private:
    float MaxScroll(int count, float screenH) const; // max scroll offset; internal to scroll/scrollbar math

    ScrollableListConfig m_cfg;
    float m_scroll = 0.0f;
    int m_hovered = -1; // index of the card under the cursor, or -1
};

// Geometry parameters for a WidgetGroup. Every field has a sensible default;
// the minimum needed to get a working layout is m_bounds + m_mode plus either
// m_defaultItemW/m_defaultItemH or per-slot sizes via SetSlotSize().
struct WidgetGroupConfig {
    enum class Mode : uint8_t { Horizontal, Vertical, Grid };
    enum class Pack : uint8_t { Start, Center, End };
    enum class Align : uint8_t { Start, Center, End, Stretch };

    Mode m_mode = Mode::Vertical;

    // Primary-axis packing: where items sit within the content area.
    // Vertical mode: Start = top-down, End = bottom-up, Center = centered.
    // Horizontal mode: Start = left-to-right, End = right-to-left.
    Pack m_pack = Pack::Start;

    // Cross-axis alignment within the content area.
    // Vertical mode: controls horizontal placement (Start=left, Center, End=right, Stretch=fill width).
    // Horizontal mode: controls vertical placement (Start=top, Center, End=bottom, Stretch=fill height).
    // Grid mode: m_align = cell-X alignment, m_alignV = cell-Y alignment.
    Align m_align = Align::Start;
    Align m_alignV = Align::Start;

    Rectangle m_bounds = {};

    // Padding inside m_bounds; the remaining area is the content area where items are placed.
    float m_padTop = 0.0f;
    float m_padBottom = 0.0f;
    float m_padLeft = 0.0f;
    float m_padRight = 0.0f;

    void SetPadding(float all) {
        m_padTop = all;
        m_padBottom = all;
        m_padLeft = all;
        m_padRight = all;
    }
    void SetPadding(float horiz, float vert) {
        m_padTop = vert;
        m_padBottom = vert;
        m_padLeft = horiz;
        m_padRight = horiz;
    }

    float m_gapX = 0.0f; // horizontal gap between items
    float m_gapY = 0.0f; // vertical gap between items

    int m_columns = 1;           // Grid mode: fixed column count; rows = ceil(visibleCount / m_columns)
    float m_defaultItemW = 0.0f; // 0 = caller sets per-slot sizes, or Stretch fills the content area
    float m_defaultItemH = 0.0f;
};

// Generic layout container for any widget with a Rectangle field. Manages a vector of
// slots — each with size, visibility, and a computed rect — and lays them out according
// to the config. Callers copy slot rects into their widget members after Layout().
//
// Usage:
//   WidgetGroup g;
//   g.m_config.m_mode = WidgetGroupConfig::Mode::Horizontal;
//   g.m_config.m_bounds = {x, y, w, h};
//   g.m_config.m_defaultItemW = 64;
//   g.m_config.m_defaultItemH = 64;
//   g.m_config.m_gapX = 4;
//   g.SetCount(buttons.size());
//   g.Layout();
//   for (int i = 0; i < g.Count(); i++)
//       buttons[i].m_rect = g[i].m_rect;
class WidgetGroup {
public:
    struct Slot {
        float m_width = 0.0f;  // 0 = use m_config.m_defaultItemW
        float m_height = 0.0f; // 0 = use m_config.m_defaultItemH
        bool m_visible = true;
        Rectangle m_rect = {}; // set by Layout()
    };

    WidgetGroupConfig m_config;

    // Resize the slot vector. New slots are default-constructed (visible, zero size).
    void SetCount(int count);

    int Count() const { return static_cast<int>(m_slots.size()); }

    // Per-slot overrides. Index must be in [0, Count()).
    void SetSlotSize(int index, float w, float h);
    void SetSlotVisible(int index, bool visible);
    bool IsSlotVisible(int index) const;

    const Slot& operator[](int index) const { return m_slots[index]; }

    // Compute m_rect for every visible slot. Call after changing config, slot sizes,
    // or slot visibility.
    void Layout();

    // Bounds minus padding — the area items are placed within.
    Rectangle ContentArea() const;

    // Count of slots with m_visible == true.
    int VisibleCount() const;

    // Total extent of visible items including gaps along the primary layout axis.
    // Useful for sizing m_bounds dynamically.
    float TotalContentWidth() const;
    float TotalContentHeight() const;

    // Returns the index of the first visible slot whose computed rect contains `point`,
    // or -1. Non-visible slots are skipped.
    int HitTest(Vector2 point) const;

    // Exclusive selection: sets m_selectedSlot to `index`, clearing any previous selection.
    // Pass -1 to clear. Callers can read m_selectedSlot directly in their draw loop.
    void SelectSlot(int index) { m_selectedSlot = index; }
    int SelectedSlot() const { return m_selectedSlot; }
    int m_selectedSlot = -1; // index of the exclusively-selected slot, or -1

private:
    std::vector<Slot> m_slots;

    void layoutVertical();
    void layoutHorizontal();
    void layoutGrid();

    // Resolve effective item dimensions from slot override, config default, and content area.
    float resolveWidth(const Slot& slot, float contentW) const;
    float resolveHeight(const Slot& slot, float contentH) const;
};
