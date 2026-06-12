#include <content/datapack_registry.hpp>
#include <toml++/toml.hpp>
#include <algorithm>
#include <filesystem>
#include <iostream>

namespace {
    // Name of the directory holding all installed packs, relative to project root.
    constexpr const char* kDatapacksDir = "datapacks";
    // Required metadata file inside each pack directory.
    constexpr const char* kManifest = "pack.toml";

    // Reads a required string field; returns false (and logs) if missing or empty.
    bool ReadRequired(const toml::table& tbl, const char* key, std::string& out) {
        auto value = tbl[key].value<std::string>();
        if (!value || value->empty()) {
            std::cerr << "DatapackRegistry: pack.toml missing required '" << key << "'\n";
            return false;
        }
        out = *value;
        return true;
    }
}

void DatapackRegistry::Scan(FileStore& fileStore) {
    UnloadIcons();
    m_packs.clear();
    m_rootPath = fileStore.GetRootPath();

    std::filesystem::path base = std::filesystem::path(m_rootPath) / kDatapacksDir;
    if (!std::filesystem::is_directory(base)) {
        std::cerr << "DatapackRegistry: no '" << kDatapacksDir << "' directory found\n";
        return;
    }

    for (const auto& entry : std::filesystem::directory_iterator(base)) {
        if (!entry.is_directory()) continue;

        std::string dirName = entry.path().filename().string();
        std::string rootDir = std::string(kDatapacksDir) + "/" + dirName;
        std::string manifest = rootDir + "/" + kManifest;

        // A directory without a manifest is not a pack — skip quietly.
        if (!fileStore.Exists(manifest)) {
            std::cerr << "DatapackRegistry: skipping '" << dirName << "' (no " << kManifest << ")\n";
            continue;
        }

        // LoadToml already logs parse errors and returns an empty table on failure.
        toml::table tbl = fileStore.LoadToml(manifest);

        Datapack pack;
        pack.m_rootDir = rootDir;
        bool ok = ReadRequired(tbl, "name", pack.m_name)
               && ReadRequired(tbl, "author", pack.m_author)
               && ReadRequired(tbl, "version", pack.m_version)
               && ReadRequired(tbl, "description", pack.m_description)
               && ReadRequired(tbl, "icon", pack.m_iconPath);
        if (!ok) {
            std::cerr << "DatapackRegistry: skipping '" << dirName << "' (invalid manifest)\n";
            continue;
        }

        m_packs.push_back(std::move(pack));
        std::cout << "DatapackRegistry: found '" << m_packs.back().m_name << "'\n";
    }

    // Stable alphabetical order so the selection list does not depend on the
    // filesystem's directory iteration order.
    std::sort(m_packs.begin(), m_packs.end(),
              [](const Datapack& a, const Datapack& b) { return a.m_name < b.m_name; });
}

void DatapackRegistry::LoadIcons() {
    for (auto& pack : m_packs) {
        if (pack.m_icon.id != 0) continue; // already loaded

        std::filesystem::path iconPath =
            std::filesystem::path(m_rootPath) / pack.m_rootDir / pack.m_iconPath;
        if (!std::filesystem::exists(iconPath)) {
            std::cerr << "DatapackRegistry: icon not found for '" << pack.m_name << "'\n";
            continue; // leaves m_icon.id == 0; the selection screen draws a placeholder
        }
        pack.m_icon = ::LoadTexture(iconPath.string().c_str());
    }
}

void DatapackRegistry::UnloadIcons() {
    for (auto& pack : m_packs) {
        if (pack.m_icon.id == 0) continue;
        ::UnloadTexture(pack.m_icon);
        pack.m_icon = {};
    }
}
