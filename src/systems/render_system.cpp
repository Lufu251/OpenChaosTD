#include <systems/render_system.hpp>
#include <content/tile_factory.hpp>
#include <engine/core/text_renderer.hpp>
#include <engine/util/file_store.hpp>
#include <toml++/toml.hpp>

#include <raymath.h>

void RenderSystem::Load(FileStore& fileStore) {
    // Health bar styling is themed with the rest of the UI in config/hud.toml.
    if (fileStore.Exists("config/hud.toml")) {
        const toml::table tbl = fileStore.LoadToml("config/hud.toml");
        if (const toml::table* hb = tbl["health_bar"].as_table()) {
            m_hbWidth    = (*hb)["width"].value_or(m_hbWidth);
            m_hbHeight   = (*hb)["height"].value_or(m_hbHeight);
            m_hbPadding  = (*hb)["padding"].value_or(m_hbPadding);
            m_hbRound    = (*hb)["roundness"].value_or(m_hbRound);
            m_hbSegs     = (*hb)["segments"].value_or(m_hbSegs);
            if (const toml::array* c = (*hb)["bgColor"].as_array(); c && c->size() >= 4)
                m_hbBgColor = {
                    static_cast<unsigned char>((*c)[0].value_or(0)),
                    static_cast<unsigned char>((*c)[1].value_or(0)),
                    static_cast<unsigned char>((*c)[2].value_or(0)),
                    static_cast<unsigned char>((*c)[3].value_or(255))};
            if (const toml::array* sc = (*hb)["shieldColor"].as_array(); sc && sc->size() >= 4)
                m_hbShieldColor = {
                    static_cast<unsigned char>((*sc)[0].value_or(0)),
                    static_cast<unsigned char>((*sc)[1].value_or(0)),
                    static_cast<unsigned char>((*sc)[2].value_or(0)),
                    static_cast<unsigned char>((*sc)[3].value_or(255))};
            if (const toml::array* bc = (*hb)["barrierColor"].as_array(); bc && bc->size() >= 4)
                m_hbBarrierColor = {
                    static_cast<unsigned char>((*bc)[0].value_or(0)),
                    static_cast<unsigned char>((*bc)[1].value_or(0)),
                    static_cast<unsigned char>((*bc)[2].value_or(0)),
                    static_cast<unsigned char>((*bc)[3].value_or(255))};
        }
    }

    // Camera zoom levels are a display setting, alongside fps/hudScale in config/settings.toml.
    if (fileStore.Exists("config/settings.toml")) {
        const toml::table tbl = fileStore.LoadToml("config/settings.toml");
        if (const toml::array* zl = tbl["zoom"]["levels"].as_array(); zl && !zl->empty()) {
            m_zoomLevels.clear();
            for (auto&& node : *zl)
                if (auto v = node.value<float>()) m_zoomLevels.push_back(*v);
            // Clamp the current zoom index to the new range
            m_zoomIndex = Clamp(m_zoomIndex, 0, static_cast<int>(m_zoomLevels.size()) - 1);
        }
    }
}

void RenderSystem::DrawMap(const Map& map, const TileFactory& tileFactory, Resources& assets){
    for (int y = 0; y < map.GetRows(); y++) {
        for (int x = 0; x < map.GetCols(); x++) {
            const Tile& tile = map.Get(x, y);

            // Look up the tile definition; skip tiles whose ID is not in the registry
            // (e.g. from a stale save after a datapack change).
            if (!tileFactory.Has(tile.m_tileId))
                continue;
            const auto& def = tileFactory.Get(tile.m_tileId);

            // Pick from the definition's texture array using this tile's stored index.
            int idx = def.textures.empty() ? 0 : tile.m_textureIndex % static_cast<int>(def.textures.size());
            std::string texKey = def.textures.empty() ? std::string{} : def.textures[idx];

            if (!assets.HasTexture(texKey))
                continue;

            DrawTexture(assets.GetTexture(texKey), map.TileToWorld(x, y).x, map.TileToWorld(x, y).y, WHITE);
        }
    }
}

void RenderSystem::DrawPaths(const Map& map){
    for (auto& path : map.GetPaths()) {
        for (size_t i=0; i < path.size(); i++) {
            if(i +1 >= path.size()) continue;
            DrawLineEx(path[i], path[i +1], 2, {200,41,55,150});
        }
    }
}

void RenderSystem::DebugDrawMap(const Map& map){
    int tileSize = map.GetTileSize();
    int halfTileSize = map.GetTileSize() /2;

    for (int y = 0; y < map.GetRows(); y++) {
        for (int x = 0; x < map.GetCols(); x++) {
            // Draw flowfield flow direction and distance
            if(map.GetPathMesh().Get(x, y).m_distance != std::numeric_limits<int>::max()){
                // Distance
                Text::Draw(TextFormat("%i", map.GetPathMesh().Get(x, y).m_distance), map.TileToWorld(x, y).x + 1, map.TileToWorld(x, y).y + 1, 6, BLACK, Text::Kind::Number);

                // Flow direction
                std::pair<int, int> end = map.GetPathMesh().Get(x, y).m_predecessor;;
                DrawLine(x * tileSize +halfTileSize, y * tileSize +halfTileSize, end.first * tileSize +halfTileSize, end.second * tileSize +halfTileSize, BLACK);
            }
        }
    }
}

void RenderSystem::DrawTowers(const DenseSlotMap<Tower>& towers, Resources& assets){
    for (auto& tower : towers) {
        Texture2D& texture = assets.GetTexture(tower.m_presentation.m_texture);
        float hw = static_cast<float>(texture.width)  / 2.0f;
        float hh = static_cast<float>(texture.height) / 2.0f;

        float flashRatio = tower.m_animation.m_attackFlashRatio;
        Color tint = (flashRatio > 0.0f) ? ColorLerp(WHITE, ORANGE, flashRatio) : WHITE;
        DrawTextureV(texture, {tower.m_position.x - hw, tower.m_position.y - hh}, tint);

        // Draw level number at bottom-right of sprite once any upgrade has been purchased
        if (tower.m_upgrades && !tower.m_upgrades->empty() && tower.m_level > 0) {
            bool isMax = tower.m_level >= static_cast<int>(tower.m_upgrades->size());
            const char* lvlText = TextFormat("%d", tower.m_level + 1);
            constexpr int kFontSize = 10;
            int tw = Text::Measure(lvlText, kFontSize, Text::Kind::Number);
            Text::Draw(lvlText,
                     static_cast<int>(tower.m_position.x + hw) - tw - 1,
                     static_cast<int>(tower.m_position.y + hh) - kFontSize - 1,
                     kFontSize, isMax ? GOLD : WHITE, Text::Kind::Number);
        }
    }
}

void RenderSystem::DrawTowerRange(Vector2 position, float radius, Color color) {
    if (radius <= 0.0f) return;
    DrawCircleV(position, radius, {color.r, color.g, color.b, 30});
    DrawCircleLinesV(position, radius, color);
}

void RenderSystem::DrawRangeIndicator(DenseSlotMap<Tower>::Key selectedKey, const Map& map, const DenseSlotMap<Tower>& towers, Vector2 mouseWorld) {
    if (selectedKey != DenseSlotMap<Tower>::INVALID_KEY) {
        if (const Tower* t = towers.Get(selectedKey))
            if (const AttackModule* a = t->GetAttack())
                DrawTowerRange(t->m_position, a->m_liveRange, {255, 200, 50, 220});
        return;
    }

    // Show range of whichever placed tower the mouse is hovering
    int hx, hy;
    if (map.WorldToTile(mouseWorld, hx, hy)) {
        const Tile& tile = map.Get(hx, hy);
        if (tile.m_towerKey != DenseSlotMap<Tower>::INVALID_KEY) {
            if (const Tower* t = towers.Get(tile.m_towerKey))
                if (const AttackModule* a = t->GetAttack())
                    DrawTowerRange(t->m_position, a->m_liveRange, {255, 255, 255, 80});
        }
    }
}

void RenderSystem::DrawGhostTower(Vector2 position, float radius, Texture2D& texture) {
    float hw = static_cast<float>(texture.width)  / 2.0f;
    float hh = static_cast<float>(texture.height) / 2.0f;
    DrawTextureV(texture, {position.x - hw, position.y - hh}, {255, 255, 255, 140});
    DrawTowerRange(position, radius, {255, 255, 255, 140});
}

void RenderSystem::DrawEnemies(const DenseSlotMap<Enemy>& enemies, Resources& assets) {
    for (auto& enemy : enemies) {
        Texture2D& texture = assets.GetTexture(enemy.m_presentation.m_texture);
        float hw = static_cast<float>(texture.width)  / 2.0f;
        float hh = static_cast<float>(texture.height) / 2.0f;

        DrawTextureV(texture, {enemy.m_position.x - hw, enemy.m_position.y - hh}, WHITE);

        // Sum defensive state from all modules so the health bar can show shield and barrier layers
        float totalShield = 0.0f;
        int totalBarrier = 0;
        for (const auto& mod : enemy.m_modules) {
            totalShield += mod->GetShield();
            totalBarrier += mod->GetBarrierHits();
        }

        // Health bar floats above the sprite; dimensions come from config/hud.toml
        DrawHealthBar({enemy.m_position.x, enemy.m_position.y + hh + 2.0f},
                      enemy.m_currentHealth, enemy.GetBaseStats()->m_maxHealth,
                      totalShield, totalBarrier);
    }
}

void RenderSystem::DrawAttacks(const std::vector<Attack>& attacks) {
    for (const auto& a : attacks) {
        const AttackVisual& v = a.m_visual;
        float t = a.Progress();

        switch (v.m_style) {
            case AttackStyle::Line:
                for (const auto& target : v.m_targetPositions)
                    DrawLineEx(v.m_origin, target, 1.5f, ColorAlpha(v.m_color, t));
                break;
            case AttackStyle::Ring:
                // Ring expands from tower outward to full radius
                float r = (1.0f - t) * v.m_radius;
                DrawCircleV(v.m_origin, r, ColorAlpha(v.m_color, t * 0.12f));
                DrawCircleLinesV(v.m_origin, r, ColorAlpha(v.m_color, t * 0.9f));
                break;
        }
    }
}

void RenderSystem::DebugDrawEnemies(const DenseSlotMap<Enemy>& enemies) {
    for (auto& enemy : enemies) {
        Text::Draw(
            TextFormat("%.2f", enemy.m_progress),
            static_cast<int>(enemy.m_position.x) + 6,
            static_cast<int>(enemy.m_position.y) - 18,
            8, LIME
        );
    }
}

void RenderSystem::CenterCamera(Map& map, Screen& renderer){
    CenterCamera(map, {0.0f, 0.0f,
        static_cast<float>(renderer.GetGameWidth()),
        static_cast<float>(renderer.GetGameHeight())});
}

void RenderSystem::CenterCamera(Map& map, Rectangle viewport){
    float mapW = static_cast<float>(map.GetCols() * map.GetTileSize());
    float mapH = static_cast<float>(map.GetRows() * map.GetTileSize());
    m_camera.target   = {mapW / 2.f, mapH / 2.f};
    m_camera.offset   = {viewport.x + viewport.width / 2.f, viewport.y + viewport.height / 2.f};
    m_camera.zoom     = 1.0f;
    m_camera.rotation = 0.0f;
    m_zoomIndex = Clamp(1, 0, static_cast<int>(m_zoomLevels.size()) - 1);
}

void RenderSystem::ControlCamera(float& dt, Input& input){
    // ------------------------------
    // Moving Camera
    // ------------------------------
    Vector2 direction{0,0};

    // Move camera with keyboard
    if(input.IsDown("Up")) direction.y ++;
    if(input.IsDown("Down")) direction.y --;
    if(input.IsDown("Right")) direction.x --;
    if(input.IsDown("Left")) direction.x ++;
    direction *= 300 * dt;

    // Move camera by draging
    if(input.IsMouseDown(MOUSE_RIGHT_BUTTON)){
        direction = (input.GetMousePosition() -m_mousePositionLast) / m_camera.zoom;
    }

    m_camera.target -= direction;

    // Update m_mousePositionLast
    m_mousePositionLast = input.GetMousePosition();

    // ------------------------------
    // Zooming Camera
    // ------------------------------
    float wheel = input.GetMouseWheelDelta();
    if(wheel != 0){
        Vector2 mouseScreen = input.GetMousePosition();

        // 1. Where in the world is the mouse RIGHT NOW?
        Vector2 mouseWorld = input.GetWorldMousePosition(m_camera);

        // 2. Shift offset to mouse (makes mouse the anchor)
        m_camera.offset = mouseScreen;
        m_camera.target = mouseWorld;

        // 3. Apply zoom
        m_zoomIndex += wheel;
        m_zoomIndex = Clamp(m_zoomIndex, 0, static_cast<int>(m_zoomLevels.size()) - 1);

        m_camera.zoom = m_zoomLevels[m_zoomIndex];
    }
}

// Helper: interpolate color green -> yellow -> red based on health ratio
static Color HealthBarColor(float ratio) {
    if (ratio > 0.5f) {
        // green to yellow
        float t = (ratio - 0.5f) * 2.0f;
        return ColorLerp(YELLOW, GREEN, t);
    } else {
        // yellow to red
        float t = ratio * 2.0f;
        return ColorLerp(RED, YELLOW, t);
    }
}

void RenderSystem::DrawHealthBar(Vector2 worldPos, float currentHealth, float maxHealth,
                                  float totalShield, int totalBarrierHits) {
    if (maxHealth <= 0.0f) return;

    float x = worldPos.x - m_hbWidth / 2.0f;
    float y = worldPos.y - m_hbHeight / 2.0f;

    // Barrier is the outermost defensive layer: draw a thin gold outline around the bar
    if (totalBarrierHits > 0)
        DrawRectangleRounded({x - 1.0f, y - 1.0f, m_hbWidth + 2.0f, m_hbHeight + 2.0f},
                             m_hbRound, m_hbSegs, m_hbBarrierColor);

    // Dark background
    DrawRectangleRounded({x, y, m_hbWidth, m_hbHeight}, m_hbRound, m_hbSegs, m_hbBgColor);

    float innerW = m_hbWidth - m_hbPadding * 2.0f;
    float innerH = m_hbHeight - m_hbPadding * 2.0f;

    // Shield fill (blue) — absorbs damage before health
    float shieldRatio = Clamp(totalShield / maxHealth, 0.0f, 1.0f);
    float shieldW = innerW * shieldRatio;
    if (shieldW > 0.0f)
        DrawRectangleRec({x + m_hbPadding, y + m_hbPadding, shieldW, innerH}, m_hbShieldColor);

    // Health fill (green→yellow→red) — innermost layer, drawn after shield
    float healthRatio = Clamp(currentHealth / maxHealth, 0.0f, 1.0f);
    float remainingW = innerW - shieldW;
    float healthW = remainingW * healthRatio;
    if (healthW > 0.0f)
        DrawRectangleRec({x + m_hbPadding + shieldW, y + m_hbPadding, healthW, innerH},
                         HealthBarColor(healthRatio));
}