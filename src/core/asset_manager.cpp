#include <core/asset_manager.hpp>
#include <raylib.h>
#include <iostream>
#include <stdexcept>
#include <filesystem>


// Set Asset Path
void AssetManager::SetAssetPath(const std::string& assetPath){
    m_assetPath = assetPath;
}

// Path resolution
std::string AssetManager::ResolvePath(const std::string& relativePath) const {
    if (m_assetPath.empty())
        throw std::runtime_error("AssetManager: SearchAssetPath() must be called before loading assets");

    return (std::filesystem::path(m_assetPath) / relativePath).string();
}

// Load
void AssetManager::LoadTexture(const std::string& key, const std::string& relativePath) {
    if (m_textures.count(key)) return;

    std::string fullPath = ResolvePath(relativePath);
    Texture2D tex = ::LoadTexture(fullPath.c_str());
    if (tex.id == 0)
        throw std::runtime_error("AssetManager: failed to load texture '" + fullPath + "'");

    m_textures[key] = tex;
}

void AssetManager::LoadSound(const std::string& key, const std::string& relativePath) {
    if (m_sounds.count(key)) return;

    std::string fullPath = ResolvePath(relativePath);
    Sound sfx = ::LoadSound(fullPath.c_str());
    if (sfx.frameCount == 0)
        throw std::runtime_error("AssetManager: failed to load sound '" + fullPath + "'");

    m_sounds[key] = sfx;
}

void AssetManager::LoadFont(const std::string& key, const std::string& relativePath, int fontSize) {
    if (m_fonts.count(key)) return;

    std::string fullPath = ResolvePath(relativePath);
    Font font = ::LoadFontEx(fullPath.c_str(), fontSize, nullptr, 0);
    if (font.texture.id == 0)
        throw std::runtime_error("AssetManager: failed to load font '" + fullPath + "'");

    m_fonts[key] = font;
}

void AssetManager::LoadMusic(const std::string& key, const std::string& relativePath) {
    if (m_music.count(key)) return;

    std::string fullPath = ResolvePath(relativePath);
    Music music = ::LoadMusicStream(fullPath.c_str());
    if (music.frameCount == 0)
        throw std::runtime_error("AssetManager: failed to load music '" + fullPath + "'");

    m_music[key] = music;
}

// Retrieve
Texture2D& AssetManager::GetTexture(const std::string& key) {
    auto it = m_textures.find(key);
    if (it == m_textures.end())
        throw std::runtime_error("AssetManager: texture key '" + key + "' not found");
    return it->second;
}

Sound& AssetManager::GetSound(const std::string& key) {
    auto it = m_sounds.find(key);
    if (it == m_sounds.end())
        throw std::runtime_error("AssetManager: sound key '" + key + "' not found");
    return it->second;
}

Font& AssetManager::GetFont(const std::string& key) {
    auto it = m_fonts.find(key);
    if (it == m_fonts.end())
        throw std::runtime_error("AssetManager: font key '" + key + "' not found");
    return it->second;
}

Music& AssetManager::GetMusic(const std::string& key) {
    auto it = m_music.find(key);
    if (it == m_music.end())
        throw std::runtime_error("AssetManager: music key '" + key + "' not found");
    return it->second;
}

//  Query
bool AssetManager::HasTexture(const std::string& key) const { return m_textures.count(key) > 0; }
bool AssetManager::HasSound(const std::string& key)   const { return m_sounds.count(key)   > 0; }
bool AssetManager::HasFont(const std::string& key)    const { return m_fonts.count(key)    > 0; }
bool AssetManager::HasMusic(const std::string& key)   const { return m_music.count(key)    > 0; }

// Shutdown
void AssetManager::Shutdown() {
    for (auto& [key, tex]  : m_textures) UnloadTexture(tex);
    for (auto& [key, sfx]  : m_sounds)   UnloadSound(sfx);
    for (auto& [key, font] : m_fonts)    UnloadFont(font);
    for (auto& [key, music]: m_music)    UnloadMusicStream(music);

    m_textures.clear();
    m_sounds.clear();
    m_fonts.clear();
    m_music.clear();
}