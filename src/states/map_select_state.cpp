#include <states/map_select_state.hpp>
#include <states/datapack_select_state.hpp>
#include <states/play_state.hpp>
#include <engine/core/text_renderer.hpp>
#include <hud/hud_theme.hpp>
#include <app/game.hpp>
#include <toml++/toml.hpp>
#include <raylib.h>
#include <memory>
#include <string>

namespace {
    constexpr Color kAutoTint = {45, 55, 70, 255}; // distinct tint for the procedural-map card

    constexpr float kThumbW = 160.0f; // preview column width inside a card
}

// --- Lifecycle ---------------------------------------------------------------

void MapSelectState::OnEnter(Game& game) {
    RebuildList(game); // also resets the scroll/hover state

    float screenW = static_cast<float>(game.GetScreen().GetGameWidth());
    float screenH = static_cast<float>(game.GetScreen().GetGameHeight());
    m_backButton.m_label = "BACK";
    m_backButton.m_rect = {screenW / 2.0f - 80.0f, m_list.ListBottom(screenH) + 18.0f, 160.0f, 44.0f};
}

void MapSelectState::OnExit(Game& /*game*/) {
    UnloadPreviews();
}

// --- Setup -------------------------------------------------------------------

void MapSelectState::RebuildList(Game& game) {
    UnloadPreviews();
    m_entries.clear();
    m_list.Reset();

    FileStore& fs = game.GetFileStore();
    std::string mapsDir = game.GetActiveMapsDir();

    for (const std::string& folder : fs.ListSubfolders(mapsDir)) {
        std::string mapDir = mapsDir + "/" + folder;
        std::string tomlPath = mapDir + "/map.toml";
        if (!fs.Exists(tomlPath)) continue;

        toml::table t = fs.LoadToml(tomlPath);
        if (t.empty()) continue; // missing/corrupt — skip

        MapEntry e;
        e.m_folder = folder;
        e.m_name = t["meta"]["name"].value_or(folder);
        e.m_description = t["meta"]["description"].value_or(std::string{});

        // Preview: bytes come from disk (desktop) or VFS/localStorage (web), so the
        // image pipeline is identical on both platforms.
        std::vector<unsigned char> bytes = fs.LoadBytes(mapDir + "/map.png");
        if (!bytes.empty()) {
            Image img = LoadImageFromMemory(".png", bytes.data(), static_cast<int>(bytes.size()));
            if (img.data != nullptr) {
                e.m_preview = LoadTextureFromImage(img);
                UnloadImage(img);
                e.m_hasPreview = e.m_preview.id != 0;
            }
        }
        m_entries.push_back(std::move(e));
    }

    // The procedural option is always offered, as the final card.
    MapEntry autoEntry;
    autoEntry.m_isAuto = true;
    autoEntry.m_name = "Auto-Generated";
    autoEntry.m_description = "A fresh, procedurally generated map.";
    m_entries.push_back(std::move(autoEntry));
}

void MapSelectState::UnloadPreviews() {
    for (MapEntry& e : m_entries) {
        if (e.m_hasPreview) {
            UnloadTexture(e.m_preview);
            e.m_preview = {};
            e.m_hasPreview = false;
        }
    }
}

// --- Selection ---------------------------------------------------------------

void MapSelectState::SelectEntry(Game& game, int index) {
    if (index < 0 || index >= static_cast<int>(m_entries.size())) return;
    const MapEntry& e = m_entries[index];

    if (e.m_isAuto)
        game.GetGameData().m_selectedMapDir.clear();
    else
        game.GetGameData().m_selectedMapDir = game.GetActiveMapsDir() + "/" + e.m_folder;

    game.ChangeState(std::make_unique<PlayingState>());
}

// --- Input -------------------------------------------------------------------

void MapSelectState::ProcessInput(Game& game, float /*dt*/) {
    Vector2 mouse = game.GetInput().GetMousePosition();
    bool clicked = game.GetInput().IsMousePressed(MOUSE_LEFT_BUTTON);
    float screenW = static_cast<float>(game.GetScreen().GetGameWidth());
    float screenH = static_cast<float>(game.GetScreen().GetGameHeight());
    int count = static_cast<int>(m_entries.size());

    m_list.ProcessScroll(game.GetInput().GetMouseWheelDelta(), count, screenH);

    m_backButton.Update(mouse, clicked);
    if (m_backButton.IsClicked() || game.GetInput().IsPressed("Cancel")) {
        if (m_backButton.IsClicked()) game.GetSoundSystem().PlaySfx("button_click");
        game.ChangeState(std::make_unique<DatapackSelectState>(DatapackSelectState::Intent::Play));
        return;
    }

    int chosen = m_list.ProcessHover(mouse, clicked, count, screenW, screenH);
    if (chosen >= 0) {
        game.GetSoundSystem().PlaySfx("button_click");
        SelectEntry(game, chosen);
    }
}

void MapSelectState::Update(Game& /*game*/, float /*dt*/) {}

// --- Draw --------------------------------------------------------------------

void MapSelectState::Draw(Game& game) {
    float screenW = static_cast<float>(game.GetScreen().GetGameWidth());
    float screenH = static_cast<float>(game.GetScreen().GetGameHeight());
    float listTop = m_list.ListTop();
    float listBottom = m_list.ListBottom(screenH);

    ClearBackground(Hud::kStateBackground);

    int count = static_cast<int>(m_entries.size());
    for (int i = 0; i < count; i++) {
        const MapEntry& entry = m_entries[i];
        Rectangle card = m_list.CardRect(i, screenW, screenH);

        // Cull cards entirely outside the visible band.
        if (card.y + card.height < listTop || card.y > listBottom) continue;

        bool hovered = (i == m_list.Hovered());
        DrawRectangleRec(card, hovered ? kDefaultStyle.m_bgHovered : kDefaultStyle.m_bgNormal);
        DrawRectangleLinesEx(card, hovered ? kDefaultStyle.m_borderWidthActive : kDefaultStyle.m_borderWidth,
                             hovered ? kDefaultStyle.m_borderSel : kDefaultStyle.m_border);

        // Thumbnail column.
        Rectangle thumb = {card.x + kIconPad, card.y + kIconPad, kThumbW, card.height - 2.0f * kIconPad};
        if (entry.m_isAuto) {
            DrawRectangleRec(thumb, kAutoTint);
            DrawRectangleLinesEx(thumb, 1.0f, kDefaultStyle.m_border);
            DrawCenteredText("AUTO", thumb.x + thumb.width / 2.0f,
                             thumb.y + thumb.height / 2.0f - 12.0f, 24, kDefaultStyle.m_accent);
        } else if (entry.m_hasPreview) {
            DrawRectangleRec(thumb, {15, 15, 18, 255});
            DrawTextureFitted(entry.m_preview, thumb);
            DrawRectangleLinesEx(thumb, 1.0f, kDefaultStyle.m_border);
        } else {
            DrawRectangleRec(thumb, Hud::kPlaceholderBg);
            DrawRectangleLinesEx(thumb, 1.0f, kDefaultStyle.m_border);
            DrawCenteredText("no preview", thumb.x + thumb.width / 2.0f,
                             thumb.y + thumb.height / 2.0f - 8.0f, 16, Hud::kSubtle);
        }

        // Text column.
        float textX = thumb.x + thumb.width + 20.0f;
        float textRight = card.x + card.width - 20.0f;
        float textW = textRight - textX;

        Color nameColor = entry.m_isAuto ? kDefaultStyle.m_accent : kDefaultStyle.m_text;
        Text::Draw(entry.m_name.c_str(), static_cast<int>(textX), static_cast<int>(card.y + 24.0f),
                   26, nameColor);

        std::string desc = TruncateToWidth(entry.m_description, 16, textW);
        Text::Draw(desc.c_str(), static_cast<int>(textX), static_cast<int>(card.y + 64.0f), 16, Hud::kSubtle);
    }

    // Scrollbar (only when there is something to scroll).
    m_list.DrawScrollbar(count, screenW, screenH, Hud::kPlaceholderBg, kDefaultStyle.m_border);

    // Header mask + title (covers any card scrolled up into this band).
    DrawRectangle(0, 0, static_cast<int>(screenW), static_cast<int>(listTop), Hud::kStateBackground);
    DrawCenteredText("SELECT MAP", screenW / 2.0f, 40.0f, static_cast<int>(Hud::kFontStateTitle), RAYWHITE);

    // Footer mask + back button.
    DrawRectangle(0, static_cast<int>(listBottom), static_cast<int>(screenW),
                  static_cast<int>(screenH - listBottom), Hud::kStateBackground);
    m_backButton.Draw();
    m_backButton.DrawLabel(static_cast<int>(Hud::kFontMenuButton), RAYWHITE);
}
