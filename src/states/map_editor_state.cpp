#include <states/map_editor_state.hpp>
#include <states/menu_state.hpp>
#include <states/card_list.hpp>
#include <engine/core/text_renderer.hpp>
#include <hud/hud_theme.hpp>
#include <world/tile.hpp>
#include <content/tile_factory.hpp>
#include <app/game.hpp>
#include <toml++/toml.hpp>
#include <raylib.h>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <memory>
#include <string>

#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif

namespace {
    // Alpha for the translucent brush ghost and the faint grid overlay.
    constexpr unsigned char kBrushGhostAlpha = 140;
    constexpr unsigned char kGridAlpha = 30;

    // Tint for the active brush ghost / hover highlight, keyed by tile ID.
    Color BrushTint(const std::string& tileId) {
        Color c;
        if (tileId == "rock")                c = Hud::kPanelBorder;    // neutral slate
        else if (tileId == "core")           c = Hud::kHighlight;      // focus gold
        else if (tileId == "nest")           c = Hud::kStatusNegative; // danger red
        else if (tileId.find("buff") == 0)   c = Hud::kAccent;         // informational accent
        else                                 c = Hud::kStatusPositive; // ground — positive green
        c.a = kBrushGhostAlpha;
        return c;
    }
}

// --- Lifecycle ---------------------------------------------------------------

void MapEditorState::OnEnter(Game& game) {
    // Build the dynamic brush palette from the TileFactory.
    auto& factory = game.GetTileFactory();
    m_brushes.clear();
    for (const auto& id : factory.GetIds()) {
        BrushDef bd;
        bd.m_tileId = id;
        bd.m_label = id; // use the tile ID as the button label
        m_brushes.push_back(std::move(bd));
    }

    m_newMapBtn.m_label      = "NEW MAP";
    m_catalogBackBtn.m_label = "BACK";
    m_modalCreateBtn.m_label = "CREATE";
    m_modalCancelBtn.m_label = "CANCEL";
    m_validateBtn.m_label    = "VALIDATE";
    m_saveBtn.m_label        = "SAVE";
    m_editBackBtn.m_label    = "BACK";

    m_modalName.m_maxLength = 32;
    m_modalDesc.m_maxLength = 120;
    m_modalCols.m_min = kMinDim; m_modalCols.m_max = kMaxDim; m_modalCols.m_step = 1.0f;
    m_modalRows.m_min = kMinDim; m_modalRows.m_max = kMaxDim; m_modalRows.m_step = 1.0f;
    m_modalCols.m_value = kDefaultCols;
    m_modalRows.m_value = kDefaultRows;

    Layout(game);
    m_mode = Mode::Catalog;
    RebuildCatalog(game);
}

void MapEditorState::OnExit(Game& /*game*/) {
    UnloadPreviews();
}

// --- Setup / layout ----------------------------------------------------------

void MapEditorState::Layout(Game& game) {
    float gw = static_cast<float>(game.GetScreen().GetGameWidth());
    float gh = static_cast<float>(game.GetScreen().GetGameHeight());
    float footerY = gh - kFooterH;

    // Catalog footer actions.
    m_newMapBtn.m_rect      = {kMargin, footerY + 18.0f, 180.0f, 44.0f};
    m_newMapBtn.m_fontSize = 20;
    m_newMapBtn.m_labelColor = Hud::kTextPrimary;
    m_catalogBackBtn.m_rect = {gw - kMargin - 160.0f, footerY + 18.0f, 160.0f, 44.0f};
    m_catalogBackBtn.m_fontSize = 20;
    m_catalogBackBtn.m_labelColor = Hud::kTextPrimary;

    // New-map modal, centered.
    float mw = 480.0f, mh = 330.0f;
    m_modalRect = {(gw - mw) / 2.0f, (gh - mh) / 2.0f, mw, mh};
    float mx = m_modalRect.x + 30.0f;
    float my = m_modalRect.y + 64.0f;
    m_modalName.m_rect = {mx, my, mw - 60.0f, 40.0f};
    m_modalName.m_placeholder = "Name";
    m_modalDesc.m_rect = {mx, my + 64.0f, mw - 60.0f, 40.0f};
    m_modalDesc.m_placeholder = "Description";
    m_modalCols.m_rect = {mx + 90.0f, my + 138.0f, mw - 180.0f, 24.0f};
    m_modalRows.m_rect = {mx + 90.0f, my + 174.0f, mw - 180.0f, 24.0f};
    m_modalCreateBtn.m_rect = {mx, m_modalRect.y + mh - 58.0f, 170.0f, 40.0f};
    m_modalCreateBtn.m_fontSize = 18;
    m_modalCancelBtn.m_rect = {m_modalRect.x + mw - 30.0f - 170.0f, m_modalRect.y + mh - 58.0f, 170.0f, 40.0f};
    m_modalCancelBtn.m_fontSize = 18;
    m_modalCancelBtn.m_labelColor = Hud::kTextPrimary;

    // Edit palette — brush buttons (left column), sized dynamically from the factory.
    int brushCount = static_cast<int>(m_brushes.size());
    if (brushCount == 0) brushCount = 1; // avoid zero-sized group if factory is empty
    m_brushGroup.SetCount(brushCount);
    m_brushGroup.m_config.m_mode = WidgetGroupConfig::Mode::Vertical;
    m_brushGroup.m_config.m_pack = WidgetGroupConfig::Pack::Start;
    m_brushGroup.m_config.m_align = WidgetGroupConfig::Align::Stretch;
    m_brushGroup.m_config.m_bounds = {
        kPaletteX, kTopY + 30.0f, kPaletteW,
        static_cast<float>(brushCount) * kBrushBtnH + static_cast<float>(brushCount - 1) * kRowGap
    };
    m_brushGroup.m_config.m_defaultItemH = kBrushBtnH;
    m_brushGroup.m_config.m_gapY = kRowGap;
    m_brushGroup.Layout();
    for (int i = 0; i < brushCount; i++) {
        m_brushes[i].m_button.m_rect = m_brushGroup[i].m_rect;
        m_brushes[i].m_button.m_label = m_brushes[i].m_label;
        m_brushes[i].m_button.m_fontSize = 18;
        m_brushes[i].m_button.m_labelColor = Hud::kTextPrimary;
    }

    // Edit canvas + bottom action bar.
    m_canvasRect = {kCanvasX, kTopY, gw - kMargin - kCanvasX, footerY - kTopY};
    float by = footerY + 18.0f;
    m_editLeftBtns.SetCount(2);
    m_editLeftBtns.m_config.m_mode = WidgetGroupConfig::Mode::Horizontal;
    m_editLeftBtns.m_config.m_pack = WidgetGroupConfig::Pack::Start;
    m_editLeftBtns.m_config.m_align = WidgetGroupConfig::Align::Start;
    m_editLeftBtns.m_config.m_bounds = {kCanvasX, by, 336.0f, 44.0f};
    m_editLeftBtns.m_config.m_defaultItemW = 160.0f;
    m_editLeftBtns.m_config.m_defaultItemH = 44.0f;
    m_editLeftBtns.m_config.m_gapX = 16.0f;
    m_editLeftBtns.Layout();
    m_validateBtn.m_rect = m_editLeftBtns[0].m_rect;
    m_validateBtn.m_fontSize = 18;
    m_validateBtn.m_labelColor = Hud::kTextPrimary;
    m_saveBtn.m_rect = m_editLeftBtns[1].m_rect;
    m_saveBtn.m_fontSize = 18;
    m_editBackBtn.m_rect = {gw - kMargin - 160.0f, by, 160.0f, 44.0f};
    m_editBackBtn.m_fontSize = 18;
    m_editBackBtn.m_labelColor = Hud::kTextPrimary;
}

void MapEditorState::RebuildCatalog(Game& game) {
    UnloadPreviews();
    m_entries.clear();
    m_list.Reset();

    FileStore& fs = game.GetFileStore();
    std::string mapsDir = MapsDir(game);

    for (const std::string& folder : fs.ListSubfolders(mapsDir)) {
        std::string mapDir = mapsDir + "/" + folder;
        std::string tomlPath = mapDir + "/map.toml";
        if (!fs.Exists(tomlPath)) continue;

        toml::table t = fs.LoadToml(tomlPath);
        if (t.empty()) continue;

        MapEntry e;
        e.m_folder = folder;
        e.m_name = t["meta"]["name"].value_or(folder);
        e.m_description = t["meta"]["description"].value_or(std::string{});

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

    m_deleteButtons.assign(m_entries.size(), Button{});
    for (size_t i = 0; i < m_entries.size(); i++) {
        m_deleteButtons[i].m_label = "DELETE";
        m_deleteButtons[i].m_fontSize = 14;
        m_deleteButtons[i].m_labelColor = Hud::kStatusNegative;
    }
}

void MapEditorState::UnloadPreviews() {
    for (MapEntry& e : m_entries) {
        if (e.m_hasPreview) {
            UnloadTexture(e.m_preview);
            e.m_preview = {};
            e.m_hasPreview = false;
        }
    }
}

// --- Helpers -----------------------------------------------------------------

void MapEditorState::SetStatus(const std::string& msg, bool ok) {
    m_status.Set(msg);
    m_statusOk = ok;
}

void MapEditorState::SanitizeName(std::string& name) {
    name.erase(std::remove_if(name.begin(), name.end(), [](unsigned char c) {
        return !(std::isalnum(c) || c == '_' || c == '-');
    }), name.end());
}

std::string MapEditorState::MapsDir(Game& game) const {
    return game.GetActiveMapsDir();
}

std::string MapEditorState::MapDir(Game& game, const std::string& folder) const {
    return MapsDir(game) + "/" + folder;
}

// --- Catalog actions ---------------------------------------------------------

void MapEditorState::OpenMap(Game& game, int index) {
    if (index < 0 || index >= static_cast<int>(m_entries.size())) return;
    const std::string& folder = m_entries[index].m_folder;

    if (!MapSerialization::Load(game.GetFileStore(), MapDir(game, folder), m_map, m_meta,
                                game.GetTileFactory(), "grass")) {
        SetStatus("Could not load '" + folder + "'", false);
        return;
    }
    m_openFolder = folder;
    m_mode = Mode::Edit;
    m_brushIndex = 0;
    m_lastValidateOk = false;
    m_hoverX = m_hoverY = -1;
    m_render.CenterCamera(m_map, m_canvasRect);
}

void MapEditorState::DeleteMap(Game& game, int index) {
    if (index < 0 || index >= static_cast<int>(m_entries.size())) return;
    const std::string folder = m_entries[index].m_folder;
    game.GetFileStore().DeleteFolder(MapDir(game, folder));
    RebuildCatalog(game);
    SetStatus("Deleted '" + folder + "'", true);
}

void MapEditorState::ConfirmNewMap(Game& game) {
    std::string name = m_modalName.m_text;
    SanitizeName(name);
    if (name.empty()) {
        SetStatus("Enter a map name", false);
        return;
    }
    if (game.GetFileStore().Exists(MapDir(game, name) + "/map.toml")) {
        SetStatus("A map named '" + name + "' already exists", false);
        return;
    }

    int cols = static_cast<int>(std::lround(m_modalCols.m_value));
    int rows = static_cast<int>(std::lround(m_modalRows.m_value));
    m_map.Create(cols, rows, game.GetTileFactory(), "grass");

    m_meta.m_name = name;
    m_meta.m_description = m_modalDesc.m_text;
    m_openFolder = name;

    m_modalOpen = false;
    m_mode = Mode::Edit;
    m_brushIndex = 0;
    m_lastValidateOk = false;
    m_hoverX = m_hoverY = -1;
    m_render.CenterCamera(m_map, m_canvasRect);
    SetStatus("New map - paint a core, nests and a path, then save", true);
}

// --- Edit actions ------------------------------------------------------------

void MapEditorState::PaintAt(Game& game, int tx, int ty) {
    if (m_brushIndex < 0 || m_brushIndex >= static_cast<int>(m_brushes.size())) return;
    const BrushDef& brush = m_brushes[m_brushIndex];
    const std::string& tileId = brush.m_tileId;

    if (tileId == "core") {
        // Enforce a single core: clear the previous core tile back to ground first.
        std::pair<int, int> core = m_map.GetCore();
        if (m_map.GetGrid().InBounds(core.first, core.second)) {
            Tile& old = m_map.Get(core.first, core.second);
            if (old.m_tileId == "core")
                m_map.ApplyTileDef(core.first, core.second, game.GetTileFactory(), "grass");
        }
        m_map.ApplyTileDef(tx, ty, game.GetTileFactory(), tileId);
        m_map.SetCore(tx, ty);
        return;
    }

    if (tileId == "nest") {
        m_map.ApplyTileDef(tx, ty, game.GetTileFactory(), tileId);
        m_map.AddNest(tx, ty);
        return;
    }

    // All other tiles (grass, rock, buff_*): ApplyTileDef copies the modifier from
    // the tile definition automatically.
    m_map.ApplyTileDef(tx, ty, game.GetTileFactory(), tileId);
}

bool MapEditorState::Validate() {
    // Re-derive core/nests from the painted grid so stale geometry can't slip through.
    m_map.RebuildGeometryFromGrid();

    if (!m_map.GetGrid().InBounds(m_map.GetCore().first, m_map.GetCore().second)) {
        SetStatus("Validation failed: place a Core", false);
        m_lastValidateOk = false;
        return false;
    }
    if (m_map.GetNests().empty()) {
        SetStatus("Validation failed: place at least one Nest", false);
        m_lastValidateOk = false;
        return false;
    }
    if (!m_map.ValidatePathMesh()) {
        SetStatus("Validation failed: a Nest cannot reach the Core", false);
        m_lastValidateOk = false;
        return false;
    }

    SetStatus("Valid: all nests reach the core", true);
    m_lastValidateOk = true;
    return true;
}

void MapEditorState::Save(Game& game) {
    if (!Validate())
        return; // Validate already set a descriptive blocking status

    std::string dir = MapDir(game, m_openFolder);
    if (!MapSerialization::Save(game.GetFileStore(), dir, m_map, m_meta)) {
        SetStatus("Save failed - see log", false);
        return;
    }
    ExportPng(game, dir);
    SetStatus("Saved '" + m_meta.m_name + "'", true);
}

void MapEditorState::ExportPng(Game& game, const std::string& mapDir) {
    int w = m_map.GetCols() * m_map.GetTileSize();
    int h = m_map.GetRows() * m_map.GetTileSize();

    // Render the grid at 1:1 pixel scale, no camera, into an offscreen texture.
    RenderTexture2D target = LoadRenderTexture(w, h);
    BeginTextureMode(target);
    ClearBackground(Hud::g_mapEditorTheme.exportBg);
    m_render.DrawMap(m_map, game.GetTileFactory(), game.GetResources());
    EndTextureMode();

    Image img = LoadImageFromTexture(target.texture);
    ImageFlipVertical(&img); // render textures are stored bottom-up

    std::string pngPath = mapDir + "/map.png";
    ExportImage(img, game.GetFileStore().FullPath(pngPath).c_str());
    UnloadImage(img);
    UnloadRenderTexture(target);

#if defined(PLATFORM_WEB)
    // ExportImage wrote to MEMFS; copy it into localStorage (base64) so the preview
    // survives a reload, matching how the rest of FileStore persists on web.
    EM_ASM({
        var path = UTF8ToString($0);
        try {
            var data = FS.readFile(path);
            var bin = '';
            for (var i = 0; i < data.length; i++) bin += String.fromCharCode(data[i]);
            localStorage.setItem(path, btoa(bin));
        } catch (e) { console.error('map.png persist failed', e); }
    }, pngPath.c_str());
#endif
}

// --- Input -------------------------------------------------------------------

void MapEditorState::ProcessInput(Game& game, float dt) {
    if (m_mode == Mode::Catalog) {
        if (m_modalOpen) ProcessModalInput(game);
        else             ProcessCatalogInput(game);
    } else {
        ProcessEditInput(game, dt);
    }
}

// Delete button positioned in the lower-right corner of a catalog card.
static Rectangle CatalogDeleteBtnRect(Rectangle card) {
    return {card.x + card.width - 110.0f, card.y + card.height - 35.0f, 90.0f, 24.0f};
}

void MapEditorState::ProcessCatalogInput(Game& game) {
    Input& input = game.GetInput();
    Vector2 mouse = input.GetMousePosition();
    bool clicked = input.IsMousePressed(MOUSE_LEFT_BUTTON);
    float screenW = static_cast<float>(game.GetScreen().GetGameWidth());
    float screenH = static_cast<float>(game.GetScreen().GetGameHeight());
    int count = static_cast<int>(m_entries.size());

    m_list.ProcessScroll(input.GetMouseWheelDelta(), count, screenH);

    m_newMapBtn.Update(mouse, clicked);
    m_catalogBackBtn.Update(mouse, clicked);
    if (m_catalogBackBtn.IsClicked() || input.IsPressed("Cancel")) {
        if (m_catalogBackBtn.IsClicked()) game.GetSoundSystem().PlaySfx("button_click");
        game.ChangeState(std::make_unique<MenuState>());
        return;
    }
    if (m_newMapBtn.IsClicked()) {
        game.GetSoundSystem().PlaySfx("button_click");
        m_modalOpen = true;
        m_modalName.m_text.clear();
        m_modalDesc.m_text.clear();
        m_modalCols.m_value = kDefaultCols;
        m_modalRows.m_value = kDefaultRows;
        return;
    }

    // Delete buttons — process before card selection so a delete click doesn't also
    // register as a card-open. Only visible cards (inside the list band) are hit.
    float listTop = m_list.ListTop();
    float listBottom = m_list.ListBottom(screenH);
    for (int i = 0; i < count; i++) {
        Rectangle card = m_list.CardRect(i, screenW, screenH);
        if (card.y + card.height < listTop || card.y > listBottom) continue;
        m_deleteButtons[i].m_rect = CatalogDeleteBtnRect(card);
        m_deleteButtons[i].Update(mouse, clicked);
        if (m_deleteButtons[i].IsClicked()) {
            game.GetSoundSystem().PlaySfx("button_click");
            DeleteMap(game, i);
            return;
        }
    }

    // Card hover/select — clicking a card opens the map for editing.
    int chosen = m_list.ProcessHover(mouse, clicked, count, screenW, screenH);
    if (chosen >= 0) {
        game.GetSoundSystem().PlaySfx("button_click");
        OpenMap(game, chosen);
    }
}

void MapEditorState::ProcessModalInput(Game& game) {
    Input& input = game.GetInput();
    Vector2 mouse = input.GetMousePosition();
    bool pressed = input.IsMousePressed(MOUSE_LEFT_BUTTON);
    bool down = input.IsMouseDown(MOUSE_LEFT_BUTTON);

    m_modalName.Update(mouse, pressed);
    SanitizeName(m_modalName.m_text);
    m_modalDesc.Update(mouse, pressed);
    m_modalCols.Update(mouse, down);
    m_modalRows.Update(mouse, down);
    m_modalCreateBtn.Update(mouse, pressed);
    m_modalCancelBtn.Update(mouse, pressed);

    bool typing = m_modalName.IsFocused() || m_modalDesc.IsFocused();
    if (m_modalCancelBtn.IsClicked() || (input.IsPressed("Cancel") && !typing)) {
        if (m_modalCancelBtn.IsClicked()) game.GetSoundSystem().PlaySfx("button_click");
        m_modalOpen = false;
        return;
    }
    if (m_modalCreateBtn.IsClicked()) {
        game.GetSoundSystem().PlaySfx("button_click");
        ConfirmNewMap(game);
    }
}

void MapEditorState::ProcessEditInput(Game& game, float dt) {
    Input& input = game.GetInput();
    Vector2 mouse = input.GetMousePosition();
    bool pressed = input.IsMousePressed(MOUSE_LEFT_BUTTON);
    bool down = input.IsMouseDown(MOUSE_LEFT_BUTTON);

    // Brush palette — dynamic button count.
    int brushCount = static_cast<int>(m_brushes.size());
    for (int i = 0; i < brushCount; i++) {
        m_brushes[i].m_button.Update(mouse, pressed);
        if (m_brushes[i].m_button.IsClicked()) {
            game.GetSoundSystem().PlaySfx("button_click");
            m_brushIndex = i;
        }
    }
    // Bottom action bar.
    m_validateBtn.Update(mouse, pressed);
    m_saveBtn.Update(mouse, pressed);
    m_editBackBtn.Update(mouse, pressed);
    if (m_editBackBtn.IsClicked() || input.IsPressed("Cancel")) {
        if (m_editBackBtn.IsClicked()) game.GetSoundSystem().PlaySfx("button_click");
        m_mode = Mode::Catalog;
        RebuildCatalog(game);
        return;
    }
    if (m_validateBtn.IsClicked()) {
        game.GetSoundSystem().PlaySfx("button_click");
        Validate();
    }
    // SAVE is drawn disabled until the last validation passed; honor that here so a click on the
    // greyed button is a no-op instead of running Save (and surfacing a validation error toast).
    if (m_saveBtn.IsClicked() && m_lastValidateOk) {
        game.GetSoundSystem().PlaySfx("button_click");
        Save(game);
    }

    // Block grid interaction when the cursor is over the palette/bottom-bar UI.
    float gw = static_cast<float>(game.GetScreen().GetGameWidth());
    float gh = static_cast<float>(game.GetScreen().GetGameHeight());
    Rectangle paletteBand = {0.0f, kTopY, kCanvasX - 10.0f, gh - kFooterH - kTopY};
    Rectangle footerBand  = {0.0f, gh - kFooterH, gw, kFooterH};
    if (CheckCollisionPointRec(mouse, paletteBand) || CheckCollisionPointRec(mouse, footerBand))
        input.ConsumeMouseInput();

    // Canvas: hover ghost, paint on drag, camera pan/zoom.
    m_hoverX = m_hoverY = -1;
    bool overCanvas = CheckCollisionPointRec(mouse, m_canvasRect) && !input.IsMouseInputConsumed();
    if (overCanvas) {
        Vector2 world = input.GetWorldMousePosition(m_render.GetCamera());
        int tx, ty;
        if (m_map.WorldToTile(world, tx, ty)) {
            m_hoverX = tx;
            m_hoverY = ty;
            if (down) {
                PaintAt(game, tx, ty);
                m_lastValidateOk = false;
            }
        }
        m_render.ControlCamera(dt, input); // right-drag pan + wheel zoom
    }
}

void MapEditorState::Update(Game& /*game*/, float dt) {
    m_status.Update(dt);
}

// --- Draw --------------------------------------------------------------------

void MapEditorState::Draw(Game& game) {
    float gw = static_cast<float>(game.GetScreen().GetGameWidth());
    float gh = static_cast<float>(game.GetScreen().GetGameHeight());

    if (m_mode == Mode::Catalog) {
        // Catalog mode: DrawCatalog handles clear, cards, header/footer masks.
        DrawCatalog(game);
        if (m_modalOpen)
            DrawNewMapModal(game);
    } else {
        ClearBackground(Hud::kWorldBackground);
        DrawCenteredText("MAP EDITOR", gw / 2.0f, 40.0f, static_cast<int>(Hud::kFontStateTitle), Hud::kTextPrimary);
        DrawPalette(game);
        DrawEditCanvas(game);
        DrawBottomBar(game);
    }

    m_status.Draw(gw / 2.0f, gh - kFooterH - 26.0f, 18,
                  m_statusOk ? Hud::kStatusPositive : Hud::kStatusNegative);
}

void MapEditorState::DrawCatalog(Game& game) {
    ClearBackground(Hud::kWorldBackground);

    float screenW = static_cast<float>(game.GetScreen().GetGameWidth());
    float screenH = static_cast<float>(game.GetScreen().GetGameHeight());
    float listTop = m_list.ListTop();
    float listBottom = m_list.ListBottom(screenH);

    int count = static_cast<int>(m_entries.size());

    if (count == 0) {
        DrawCenteredText("No maps in this pack - click NEW MAP to create one",
                         screenW / 2.0f, screenH / 2.0f - 9.0f, 20, Hud::kTextSecondary);
    }

    for (int i = 0; i < count; i++) {
        const MapEntry& entry = m_entries[i];
        Rectangle card = m_list.CardRect(i, screenW, screenH);

        // Cull cards entirely outside the visible band.
        if (card.y + card.height < listTop || card.y > listBottom) continue;

        bool hovered = (i == m_list.Hovered());
        DrawCardFrame(card, hovered);

        // Thumbnail column.
        Rectangle thumb = CardThumbRect(card);
        DrawCardThumbnail(thumb, entry.m_hasPreview ? &entry.m_preview : nullptr);

        // Text column.
        auto textCol = CardTextColumn(card, thumb);
        float textX = textCol.x;
        float textW = textCol.width;

        Text::Draw(entry.m_name.c_str(), static_cast<int>(textX),
                   static_cast<int>(card.y + 24.0f), 26, kDefaultStyle.m_text);

        std::string desc = TruncateToWidth(entry.m_description, 16, textW);
        Text::Draw(desc.c_str(), static_cast<int>(textX),
                   static_cast<int>(card.y + 64.0f), 16, Hud::kTextSecondary);

        // Delete button (position repeated here so drawing never depends on input order).
        m_deleteButtons[i].m_rect = CatalogDeleteBtnRect(card);
        m_deleteButtons[i].Draw();
    }

    // Scrollbar (only when there is something to scroll).
    m_list.DrawScrollbar(count, screenW, screenH, Hud::kWorldBackground, kDefaultStyle.m_border);

    // Header/footer masks + title, then the action buttons over the footer mask.
    DrawListChrome(screenW, screenH, listTop, listBottom, "MAP EDITOR");
    m_newMapBtn.Draw();
    m_catalogBackBtn.Draw();
}

void MapEditorState::DrawNewMapModal(Game& /*game*/) {
    // Dim the catalog behind the modal.
    DrawRectangleRec(m_modalRect, Hud::PanelBg(Hud::kDialogAlpha));
    DrawRectangleLinesEx(m_modalRect, 2.0f, Hud::kHighlight);

    DrawCenteredText("NEW MAP", m_modalRect.x + m_modalRect.width / 2.0f,
                     m_modalRect.y + 18.0f, 26, Hud::kTextPrimary);

    m_modalName.Draw();
    m_modalDesc.Draw();

    DrawLabelInRow(TextFormat("Cols  %d", static_cast<int>(std::lround(m_modalCols.m_value))),
                   m_modalRect.x + 30.0f, m_modalCols.m_rect.y, m_modalCols.m_rect.height, 16, Hud::kTextPrimary);
    m_modalCols.Draw();
    DrawLabelInRow(TextFormat("Rows  %d", static_cast<int>(std::lround(m_modalRows.m_value))),
                   m_modalRect.x + 30.0f, m_modalRows.m_rect.y, m_modalRows.m_rect.height, 16, Hud::kTextPrimary);
    m_modalRows.Draw();

    bool canCreate = !m_modalName.m_text.empty();
    m_modalCreateBtn.m_enabled = canCreate;
    m_modalCreateBtn.m_labelColor = canCreate ? Hud::kTextPrimary : Hud::kTextDisabled;
    m_modalCreateBtn.Draw();
    m_modalCancelBtn.Draw();
}

void MapEditorState::DrawPalette(Game& /*game*/) {
    Text::Draw("BRUSH", static_cast<int>(kPaletteX), static_cast<int>(kTopY), 24, Hud::kTextHeader);

    int brushCount = static_cast<int>(m_brushes.size());
    for (int i = 0; i < brushCount; i++)
        m_brushes[i].m_button.Draw(m_brushIndex == i);
}

void MapEditorState::DrawEditCanvas(Game& game) {
    DrawRectangleRec(m_canvasRect, Hud::g_mapEditorTheme.canvasBg);

    game.GetScreen().BeginScissor(m_canvasRect);
    BeginMode2D(m_render.GetCamera());

    m_render.DrawMap(m_map, game.GetTileFactory(), game.GetResources());

    // Grid lines over the whole map.
    int cols = m_map.GetCols();
    int rows = m_map.GetRows();
    int ts = m_map.GetTileSize();
    Color gridColor = Hud::kTextPrimary; // faint translucent overlay
    gridColor.a = kGridAlpha;
    for (int x = 0; x <= cols; x++)
        DrawLine(x * ts, 0, x * ts, rows * ts, gridColor);
    for (int y = 0; y <= rows; y++)
        DrawLine(0, y * ts, cols * ts, y * ts, gridColor);

    // Active-brush ghost on the hovered tile.
    if (m_map.GetGrid().InBounds(m_hoverX, m_hoverY) && m_brushIndex >= 0
        && m_brushIndex < static_cast<int>(m_brushes.size())) {
        Vector2 wp = m_map.TileToWorld(m_hoverX, m_hoverY);
        DrawRectangle(static_cast<int>(wp.x), static_cast<int>(wp.y), ts, ts,
                      BrushTint(m_brushes[m_brushIndex].m_tileId));
        DrawRectangleLines(static_cast<int>(wp.x), static_cast<int>(wp.y), ts, ts,
                           Hud::kTextPrimary);
    }

    EndMode2D();
    game.GetScreen().EndScissor();

    DrawRectangleLinesEx(m_canvasRect, 1.0f, kDefaultStyle.m_border);

    // Map name + dimensions in the canvas corner.
    Text::Draw(TextFormat("%s   %dx%d", m_meta.m_name.c_str(), m_map.GetCols(), m_map.GetRows()),
               static_cast<int>(m_canvasRect.x + 8.0f), static_cast<int>(m_canvasRect.y + 6.0f), 16, Hud::kTextSecondary);
}

void MapEditorState::DrawBottomBar(Game& /*game*/) {
    m_validateBtn.Draw();

    bool canSave = m_lastValidateOk;
    m_saveBtn.m_enabled = canSave;
    m_saveBtn.m_labelColor = canSave ? Hud::kTextPrimary : Hud::kTextDisabled;
    m_saveBtn.Draw();

    m_editBackBtn.Draw();
}
