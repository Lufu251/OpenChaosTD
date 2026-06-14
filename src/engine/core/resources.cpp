#include <engine/core/resources.hpp>
#include <raylib.h>
#include <iostream>
#include <cassert>
#include <filesystem>
#include <unordered_set>


// Search paths
void Resources::SetAssetPath(const std::string& root) {
    m_searchPaths.assign(1, root);
}

void Resources::PushSearchPath(const std::string& root) {
    // Front = highest priority, shadows assets of the same key in lower roots.
    m_searchPaths.insert(m_searchPaths.begin(), root);
}

void Resources::PopSearchPath() {
    // Never remove the base root (the last entry).
    if (m_searchPaths.size() > 1)
        m_searchPaths.erase(m_searchPaths.begin());
}

// Path resolution
std::string Resources::ResolvePath(const std::string& relativePath) const {
    // Internal invariant: SetAssetPath() establishes the base root during startup before
    // any load occurs. Reaching here empty is a programmer error, not bad user data.
    assert(!m_searchPaths.empty() && "Resources: SetAssetPath() must be called before loading resources");

    // First search root that actually holds the file wins.
    for (const auto& root : m_searchPaths) {
        std::filesystem::path candidate = std::filesystem::path(root) / relativePath;
        if (std::filesystem::exists(candidate))
            return candidate.string();
    }

    // None matched: resolve against the base root so the error names a real path.
    return (std::filesystem::path(m_searchPaths.back()) / relativePath).string();
}

// Load
std::expected<void, std::string> Resources::LoadTexture(const std::string& key, const std::string& relativePath) {
    if (m_textures.count(key)) return {};

    std::string fullPath = ResolvePath(relativePath);
    Texture2D tex = ::LoadTexture(fullPath.c_str());
    if (tex.id == 0)
        return std::unexpected("Resources: failed to load texture '" + fullPath + "'");

    m_textures[key] = tex;
    return {};
}

std::vector<std::string> Resources::LoadTexturesFromDir(const std::string& relativeDir) {
    static const std::unordered_set<std::string> imageExts = { ".png", ".jpg", ".jpeg", ".bmp", ".tga" };

    std::vector<std::string> addedKeys;
    bool foundDir = false;

    // Walk every search root, highest priority first, so a pack texture shadows a
    // global texture of the same filename stem.
    for (const auto& root : m_searchPaths) {
        std::filesystem::path dir = std::filesystem::path(root) / relativeDir;
        if (!std::filesystem::is_directory(dir)) continue;
        foundDir = true;

        for (const auto& entry : std::filesystem::directory_iterator(dir)) {
            if (!entry.is_regular_file()) continue;
            if (!imageExts.count(entry.path().extension().string())) continue;

            std::string key = entry.path().stem().string();
            if (m_textures.count(key)) continue; // already loaded (higher-priority root won)

            Texture2D tex = ::LoadTexture(entry.path().string().c_str());
            if (tex.id == 0) {
                std::cerr << "Resources: failed to load texture '" << entry.path() << "'\n";
                continue;
            }
            m_textures[key] = tex;
            addedKeys.push_back(key);
            std::cout << "Resources: loaded texture '" << key << "'\n";
        }
    }

    if (!foundDir)
        std::cerr << "Resources: texture directory not found: '" << relativeDir << "'\n";
    return addedKeys;
}

std::vector<std::string> Resources::LoadMusicFromDir(const std::string& relativeDir) {
    static const std::unordered_set<std::string> musicExts = { ".wav", ".ogg", ".mp3", ".flac", ".xm", ".mod" };

    std::vector<std::string> addedKeys;
    bool foundDir = false;

    for (const auto& root : m_searchPaths) {
        std::filesystem::path dir = std::filesystem::path(root) / relativeDir;
        if (!std::filesystem::is_directory(dir)) continue;
        foundDir = true;

        for (const auto& entry : std::filesystem::directory_iterator(dir)) {
            if (!entry.is_regular_file()) continue;
            if (!musicExts.count(entry.path().extension().string())) continue;

            std::string key = entry.path().stem().string();
            if (m_music.count(key)) continue;

            Music music = ::LoadMusicStream(entry.path().string().c_str());
            if (music.frameCount == 0) {
                std::cerr << "Resources: failed to load music '" << entry.path() << "'\n";
                continue;
            }
            m_music[key] = music;
            addedKeys.push_back(key);
            std::cout << "Resources: loaded music '" << key << "'\n";
        }
    }

    if (!foundDir)
        std::cerr << "Resources: music directory not found: '" << relativeDir << "'\n";
    return addedKeys;
}

std::vector<std::string> Resources::LoadSoundsFromDir(const std::string& relativeDir) {
    static const std::unordered_set<std::string> soundExts = { ".wav", ".ogg", ".mp3", ".flac" };

    std::vector<std::string> addedKeys;
    bool foundDir = false;

    for (const auto& root : m_searchPaths) {
        std::filesystem::path dir = std::filesystem::path(root) / relativeDir;
        if (!std::filesystem::is_directory(dir)) continue;
        foundDir = true;

        for (const auto& entry : std::filesystem::directory_iterator(dir)) {
            if (!entry.is_regular_file()) continue;
            if (!soundExts.count(entry.path().extension().string())) continue;

            std::string key = entry.path().stem().string();
            if (m_sounds.count(key)) continue;

            Sound sfx = ::LoadSound(entry.path().string().c_str());
            if (sfx.frameCount == 0) {
                std::cerr << "Resources: failed to load sound '" << entry.path() << "'\n";
                continue;
            }
            m_sounds[key] = sfx;
            addedKeys.push_back(key);
            std::cout << "Resources: loaded sound '" << key << "'\n";
        }
    }

    if (!foundDir)
        std::cerr << "Resources: sound directory not found: '" << relativeDir << "'\n";
    return addedKeys;
}

std::expected<void, std::string> Resources::LoadSound(const std::string& key, const std::string& relativePath) {
    if (m_sounds.count(key)) return {};

    std::string fullPath = ResolvePath(relativePath);
    Sound sfx = ::LoadSound(fullPath.c_str());
    if (sfx.frameCount == 0)
        return std::unexpected("Resources: failed to load sound '" + fullPath + "'");

    m_sounds[key] = sfx;
    return {};
}

std::expected<void, std::string> Resources::LoadMusic(const std::string& key, const std::string& relativePath) {
    if (m_music.count(key)) return {};

    std::string fullPath = ResolvePath(relativePath);
    Music music = ::LoadMusicStream(fullPath.c_str());
    if (music.frameCount == 0)
        return std::unexpected("Resources: failed to load music '" + fullPath + "'");

    m_music[key] = music;
    return {};
}

// Retrieve
// Get* assume the key was loaded first (load-then-get is an internal invariant); callers
// who cannot guarantee that should gate on the matching Has* query.
Texture2D& Resources::GetTexture(const std::string& key) {
    auto it = m_textures.find(key);
    assert(it != m_textures.end() && "Resources: texture key not found");
    return it->second;
}

Sound& Resources::GetSound(const std::string& key) {
    auto it = m_sounds.find(key);
    assert(it != m_sounds.end() && "Resources: sound key not found");
    return it->second;
}

Music& Resources::GetMusic(const std::string& key) {
    auto it = m_music.find(key);
    assert(it != m_music.end() && "Resources: music key not found");
    return it->second;
}

//  Query
bool Resources::HasTexture(const std::string& key) const { return m_textures.count(key) > 0; }
bool Resources::HasSound(const std::string& key)   const { return m_sounds.count(key)   > 0; }
bool Resources::HasMusic(const std::string& key)   const { return m_music.count(key)    > 0; }

// Per-key unload
void Resources::UnloadTextureKey(const std::string& key) {
    auto it = m_textures.find(key);
    if (it == m_textures.end()) return;
    UnloadTexture(it->second);
    m_textures.erase(it);
}

void Resources::UnloadSoundKey(const std::string& key) {
    auto it = m_sounds.find(key);
    if (it == m_sounds.end()) return;
    UnloadSound(it->second);
    m_sounds.erase(it);
}

void Resources::UnloadMusicKey(const std::string& key) {
    auto it = m_music.find(key);
    if (it == m_music.end()) return;
    UnloadMusicStream(it->second);
    m_music.erase(it);
}

// Shutdown
void Resources::Shutdown() {
    for (auto& [key, tex]  : m_textures) UnloadTexture(tex);
    for (auto& [key, sfx]  : m_sounds)   UnloadSound(sfx);
    for (auto& [key, music]: m_music)    UnloadMusicStream(music);

    m_textures.clear();
    m_sounds.clear();
    m_music.clear();
}
