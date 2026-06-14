#include <states/datapack_select_state.hpp>
#include <states/menu_state.hpp>
#include <states/play_state.hpp>
#include <states/editor_select_state.hpp>
#include <states/map_select_state.hpp>
#include <states/card_list.hpp>
#include <content/datapack.hpp>
#include <engine/core/text_renderer.hpp>
#include <hud/hud_theme.hpp>
#include <app/game.hpp>
#include <raylib.h>
#include <memory>
#include <string>

void DatapackSelectState::OnEnter(Game& game) {
    // Rescan so packs dropped in since launch show up, then load their icons.
    game.GetDatapackRegistry().Scan(game.GetFileStore());
    game.GetDatapackRegistry().LoadIcons();

    m_list.Reset();

    float screenW = static_cast<float>(game.GetScreen().GetGameWidth());
    float screenH = static_cast<float>(game.GetScreen().GetGameHeight());
    m_backButton.m_label = "BACK";
    m_backButton.m_fontSize = static_cast<int>(Hud::kFontMenuButton);
    m_backButton.m_labelColor = Hud::kTextPrimary;
    m_backButton.m_rect = m_list.FooterButtonRect(screenW, screenH);
}

void DatapackSelectState::OnExit(Game& game) {
    // Selection-screen thumbnails are only needed while this screen is up.
    game.GetDatapackRegistry().UnloadIcons();
}

void DatapackSelectState::SelectPack(Game& game, int index) {
    const auto& packs = game.GetDatapackRegistry().Packs();
    if (index < 0 || index >= static_cast<int>(packs.size())) return;

    game.ActivateDatapack(packs[index]);

    if (m_intent == Intent::Edit)
        game.ChangeState(std::make_unique<EditorSelectState>());
    else if (m_intent == Intent::Continue)
        game.ChangeState(std::make_unique<PlayingState>(true));
    else
        // A new game first goes through map selection (custom map or auto-generated).
        game.ChangeState(std::make_unique<MapSelectState>());
}

void DatapackSelectState::ProcessInput(Game& game, float /*dt*/) {
    Vector2 mouse = game.GetInput().GetMousePosition();
    bool clicked = game.GetInput().IsMousePressed(MOUSE_LEFT_BUTTON);
    float screenW = static_cast<float>(game.GetScreen().GetGameWidth());
    float screenH = static_cast<float>(game.GetScreen().GetGameHeight());
    int count = static_cast<int>(game.GetDatapackRegistry().Packs().size());

    // Scroll wheel pans the list, clamped to the content extent.
    m_list.ProcessScroll(game.GetInput().GetMouseWheelDelta(), count, screenH);

    // Back button / cancel returns to the menu.
    m_backButton.Update(mouse, clicked);
    if (m_backButton.IsClicked() || game.GetInput().IsPressed("Cancel")) {
        if (m_backButton.IsClicked()) game.GetSoundSystem().PlaySfx("button_click");
        game.ChangeState(std::make_unique<MenuState>());
        return;
    }

    // Card hover/select — the widget limits hits to the visible band so clicks on the
    // masked overflow above/below the list are ignored.
    int chosen = m_list.ProcessHover(mouse, clicked, count, screenW, screenH);
    if (chosen >= 0) {
        game.GetSoundSystem().PlaySfx("button_click");
        SelectPack(game, chosen);
    }
}

void DatapackSelectState::Update(Game& /*game*/, float /*dt*/) {}

void DatapackSelectState::Draw(Game& game) {
    float screenW = static_cast<float>(game.GetScreen().GetGameWidth());
    float screenH = static_cast<float>(game.GetScreen().GetGameHeight());
    float listTop = m_list.ListTop();
    float listBottom = m_list.ListBottom(screenH);

    ClearBackground(Hud::kWorldBackground);

    const auto& packs = game.GetDatapackRegistry().Packs();
    int count = static_cast<int>(packs.size());

    if (packs.empty()) {
        DrawCenteredText("No datapacks found", screenW / 2.0f, screenH / 2.0f - 12.0f, 28, Hud::kTextSecondary);
    }

    // Cards. Overflow past the band is hidden by the header/footer masks below.
    for (int i = 0; i < count; i++) {
        const Datapack& pack = packs[i];
        Rectangle card = m_list.CardRect(i, screenW, screenH);

        // Cheap cull: skip cards entirely outside the band.
        if (card.y + card.height < listTop || card.y > listBottom) continue;

        bool hovered = (i == m_list.Hovered());
        DrawCardFrame(card, hovered);

        // Icon (or a placeholder when the pack ships none).
        float iconSize = card.height - 2.0f * kCardIconPad;
        Rectangle iconRect = { card.x + kCardIconPad, card.y + kCardIconPad, iconSize, iconSize };
        if (pack.m_icon.id != 0) {
            Rectangle src = { 0.0f, 0.0f,
                              static_cast<float>(pack.m_icon.width),
                              static_cast<float>(pack.m_icon.height) };
            DrawTexturePro(pack.m_icon, src, iconRect, { 0.0f, 0.0f }, 0.0f, Hud::kIconTint);
        } else {
            DrawRectangleRec(iconRect, Hud::kWorldBackground);
            DrawRectangleLinesEx(iconRect, 1.0f, kDefaultStyle.m_border);
        }

        // Text column.
        float textX = iconRect.x + iconSize + 20.0f;
        float textRight = card.x + card.width - 20.0f;
        float textW = textRight - textX;

        Text::Draw(pack.m_name.c_str(), static_cast<int>(textX), static_cast<int>(card.y + 16.0f),
                   26, kDefaultStyle.m_text);

        std::string meta = "by " + pack.m_author + "    v" + pack.m_version;
        Text::Draw(meta.c_str(), static_cast<int>(textX), static_cast<int>(card.y + 50.0f), 16, kDefaultStyle.m_accent);

        std::string desc = TruncateToWidth(pack.m_description, 16, textW);
        Text::Draw(desc.c_str(), static_cast<int>(textX), static_cast<int>(card.y + 78.0f), 16, Hud::kTextSecondary);
    }

    // Scrollbar (only when there is something to scroll).
    m_list.DrawScrollbar(count, screenW, screenH, Hud::kWorldBackground, kDefaultStyle.m_border);

    // Header/footer masks + title, then the back button over the footer mask.
    DrawListChrome(screenW, screenH, listTop, listBottom, "SELECT DATAPACK");
    m_backButton.Draw();
}
