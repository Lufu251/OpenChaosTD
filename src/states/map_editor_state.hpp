#pragma once

#include <states/game_state.hpp>
#include <states/state_ui_helpers.hpp>
#include <engine/systems/ui_widgets.hpp>
#include <systems/render_system.hpp>
#include <world/map.hpp>
#include <systems/serialization.hpp>
#include <string>
#include <vector>

// Data-driven map editor reached from the menu via the datapack selector. Two
// sub-modes: a catalog of the active pack's maps (open / create / delete), and a
// grid editor with paint brushes, pathing validation, TOML save, and an automatic
// map.png preview export. Modeled on ParticleEditorState (layout constants,
// sub-draw methods, status toast).
class MapEditorState : public GameState {
public:
    void OnEnter(Game& game) override;
    void OnExit(Game& game) override;

    void ProcessInput(Game& game, float dt) override;
    void Update(Game& game, float dt) override;
    void Draw(Game& game) override;

private:
    enum class Mode { Catalog, Edit };

    // --- Setup / layout ---
    void Layout(Game& game);
    void RebuildCatalog(Game& game);     // scan maps/, parse meta, load previews, build card entries

    // --- Catalog actions ---
    void UnloadPreviews();
    void OpenMap(Game& game, int index);
    void DeleteMap(Game& game, int index);
    void ConfirmNewMap(Game& game);

    // --- Edit actions ---
    void PaintAt(Game& game, int tx, int ty);
    bool Validate();                     // rebuild geometry + path checks; sets status
    void Save(Game& game);               // validate -> save toml -> export png (blocks on fail)
    void ExportPng(Game& game, const std::string& mapDir);

    // --- Helpers ---
    void SetStatus(const std::string& msg, bool ok);
    static void SanitizeName(std::string& name); // restrict to [A-Za-z0-9_-]
    std::string MapsDir(Game& game) const;
    std::string MapDir(Game& game, const std::string& folder) const;

    // --- Input sub-handlers ---
    void ProcessCatalogInput(Game& game);
    void ProcessModalInput(Game& game);
    void ProcessEditInput(Game& game, float dt);

    // --- Draw sub-parts ---
    void DrawCatalog(Game& game);
    void DrawNewMapModal(Game& game);
    void DrawEditCanvas(Game& game);
    void DrawPalette(Game& game);
    void DrawBottomBar(Game& game);

    // --- Shared state ---
    Mode m_mode = Mode::Catalog;
    StatusToast m_status;
    bool m_statusOk = true;        // tints the toast: green when ok, red on failure

    // --- Catalog ---
    struct MapEntry {
        std::string m_folder;       // subdir name under maps/
        std::string m_name;
        std::string m_description;
        Texture2D m_preview = {};
        bool m_hasPreview = false;
    };

    std::vector<MapEntry> m_entries;
    ScrollableList m_list; // scroll/hover state + card geometry (default layout)
    std::vector<Button> m_deleteButtons; // one per entry, positioned per-card in draw
    Button m_newMapBtn;
    Button m_catalogBackBtn;

    // --- New-map modal ---
    bool m_modalOpen = false;
    Rectangle m_modalRect = {};
    TextInput m_modalName;
    TextInput m_modalDesc;
    Slider m_modalCols;
    Slider m_modalRows;
    Button m_modalCreateBtn;
    Button m_modalCancelBtn;

    // --- Edit: map being edited ---
    Map m_map;
    MapSerialization::MapMeta m_meta;
    std::string m_openFolder; // folder name of the open map
    RenderSystem m_render;     // owns the editor camera + reuses DrawMap
    bool m_lastValidateOk = false;

    // --- Edit: dynamic brush palette built from TileFactory ---
    struct BrushDef {
        std::string m_tileId;
        std::string m_label;
        Button m_button;
    };
    std::vector<BrushDef> m_brushes;
    int m_brushIndex = 0; // index into m_brushes
    WidgetGroup m_brushGroup;

    // --- Edit: canvas ---
    Rectangle m_canvasRect = {};
    int m_hoverX = -1;
    int m_hoverY = -1;
    Button m_validateBtn;
    Button m_saveBtn;
    Button m_editBackBtn;
    WidgetGroup m_editLeftBtns;

    // --- Layout constants (raw virtual coords) ---
    static constexpr float kTopY       = 110.0f;
    static constexpr float kMargin     = 40.0f;
    static constexpr float kFooterH    = 80.0f;
    static constexpr float kRowGap     = 8.0f;
    static constexpr float kPaletteX   = 40.0f;
    static constexpr float kPaletteW   = 220.0f;
    static constexpr float kBrushBtnH  = 36.0f;
    static constexpr float kCanvasX    = 290.0f;
    static constexpr int   kDefaultCols = 20;
    static constexpr int   kDefaultRows = 15;
    static constexpr int   kMinDim      = 5;
    static constexpr int   kMaxDim      = 60;
};
