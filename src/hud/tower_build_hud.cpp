#include <hud/tower_build_hud.hpp>
#include <engine/core/input.hpp>
#include <engine/core/resources.hpp>
#include <raylib.h>

void TowerBuildHUD::Build(float scale, int screenW, int screenH, const std::vector<TowerBuildOption>& options) {
    HUD::Build(scale);
    m_metrics = Hud::PanelMetrics::Make(scale);
    const float btnSize = m_metrics.Scaled(64.0f);
    const float panelH  = m_metrics.Scaled(80.0f);
    const float margin  = m_metrics.margin;
    const float gap     = m_metrics.Scaled(4.0f);
    m_iconYOffset = m_metrics.Scaled(8.0f);
    m_nameYOffset = m_metrics.Scaled(18.0f);
    m_costYOffset = m_metrics.Scaled(9.0f);

    float y = screenH - btnSize - margin;
    m_panelRect = { 0.0f, screenH - panelH, static_cast<float>(screenW), panelH };

    m_buttons.clear();
    for (size_t i = 0; i < options.size(); i++) {
        BuildButton entry;
        entry.m_button.m_label = options[i].m_name;
        entry.m_button.m_rect = { margin + i * (btnSize + gap), y, btnSize, btnSize };
        entry.m_textureKey = options[i].m_textureKey;
        entry.m_cost = options[i].m_cost;
        m_buttons.push_back(std::move(entry));
    }

    m_selectedTower = "";
}

void TowerBuildHUD::ProcessInput(Input& input) {
    Vector2 mousePos{};
    bool pressed = false;
    if (!BeginInput(input, mousePos, pressed)) return;

    for (BuildButton& entry : m_buttons) {
        entry.m_button.Update(mousePos, pressed);
        if (entry.m_button.IsClicked()) {
            PlayClickSound();
            // Toggle: clicking the active type again clears the selection
            m_selectedTower = (m_selectedTower == entry.m_button.m_label) ? "" : entry.m_button.m_label;
            break;
        }
    }
}

const std::string& TowerBuildHUD::GetHoveredTower(Vector2 mousePos) const {
    static const std::string empty;
    for (const auto& entry : m_buttons)
        if (CheckCollisionPointRec(mousePos, entry.m_button.m_rect))
            return entry.m_button.m_label;
    return empty;
}

Vector2 TowerBuildHUD::GetHoveredButtonTopCenter(Vector2 mousePos) const {
    for (const auto& entry : m_buttons)
        if (CheckCollisionPointRec(mousePos, entry.m_button.m_rect))
            return { entry.m_button.m_rect.x + entry.m_button.m_rect.width / 2.0f, entry.m_button.m_rect.y };
    return {};
}

void TowerBuildHUD::Draw(const BuildBarView& view, Resources& assets) {
    if (!m_visible) return;

    DrawDockedBackground();

    int fontSize = m_metrics.fontSmall;
    for (const BuildButton& entry : m_buttons) {
        const Button& btn = entry.m_button;
        const std::string& name = btn.m_label;
        bool selected = (name == m_selectedTower);

        btn.Draw(selected);

        Texture2D& tex = assets.GetTexture(entry.m_textureKey);
        float tw = static_cast<float>(tex.width);
        float th = static_cast<float>(tex.height);
        DrawTextureV(tex, { btn.m_rect.x + (btn.m_rect.width  - tw) / 2.0f,
                            btn.m_rect.y + (btn.m_rect.height - th) / 2.0f - m_iconYOffset }, WHITE);

        int centerX = static_cast<int>(btn.m_rect.x + btn.m_rect.width / 2.0f);
        DrawTextCenteredX(name.c_str(), centerX,
            static_cast<int>(btn.m_rect.y + btn.m_rect.height - m_nameYOffset),
            fontSize, Hud::kTextMuted);

        const char* costStr = TextFormat("$%d", entry.m_cost);
        Color costColor = (view.m_gold >= entry.m_cost) ? Hud::kCostAffordable : Hud::kCostUnaffordable;
        DrawTextCenteredX(costStr, centerX,
            static_cast<int>(btn.m_rect.y + btn.m_rect.height - m_costYOffset),
            fontSize, costColor, Text::Kind::Number);
    }
}
