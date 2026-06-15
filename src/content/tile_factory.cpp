#include <content/tile_factory.hpp>
#include <engine/util/file_store.hpp>
#include <toml++/toml.hpp>
#include <iostream>

void TileFactory::Clear() {
    m_defs.clear();
    m_order.clear();
}

void TileFactory::Load(FileStore& fileStore, const std::string& dataDir) {
    Clear();

    auto data = fileStore.LoadToml(dataDir + "/tiles.toml");
    auto tiles = data["tile"].as_array();
    if (!tiles) {
        std::cerr << "TileFactory: failed to load tiles data\n";
        return;
    }

    for (auto&& entryNode : *tiles) {
        auto entry = entryNode.as_table();
        if (!entry) continue;

        TileDef def;
        def.id        = (*entry)["id"].value_or(std::string{});
        def.walkable  = (*entry)["walkable"].value_or(true);
        def.buildable = (*entry)["buildable"].value_or(true);

        // Parse textures: array "textures" takes precedence, string "texture" as fallback.
        if (auto texArr = (*entry)["textures"].as_array()) {
            for (auto&& node : *texArr)
                if (auto s = node.value<std::string>()) def.textures.push_back(*s);
        }
        if (def.textures.empty())
            if (auto s = (*entry)["texture"].value<std::string>()) def.textures.push_back(*s);

        // Parse optional embedded modifier.
        if (auto mod = (*entry)["modifier"].as_table()) {
            def.modifier.m_statKey = (*mod)["stat_key"].value_or(std::string{});
            def.modifier.m_value   = (*mod)["value"].value_or(0.0f);
            def.modifier.m_mul     = (*mod)["mul"].value_or(false);
        }

        std::string id = def.id;
        if (id.empty()) {
            std::cerr << "TileFactory: skipping entry with empty id\n";
            continue;
        }

        m_order.push_back(id);
        m_defs[id] = std::move(def);
        std::cout << "TileFactory: loaded '" << id << "'\n";
    }
}

bool TileFactory::Has(const std::string& id) const {
    return m_defs.count(id) > 0;
}

const TileFactory::TileDef& TileFactory::Get(const std::string& id) const {
    static const TileDef s_empty;
    auto it = m_defs.find(id);
    return (it != m_defs.end()) ? it->second : s_empty;
}

std::vector<std::string> TileFactory::GetBuffIds() const {
    std::vector<std::string> result;
    for (const auto& id : m_order) {
        auto it = m_defs.find(id);
        if (it != m_defs.end() && it->second.modifier.Active())
            result.push_back(id);
    }
    return result;
}
