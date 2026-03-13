#pragma once

#include <raylib.h>
#include <string>
#include <unordered_map>

class AssetManager {
public:
    AssetManager()  = default;
    ~AssetManager() { Shutdown(); }

    // Non-copyable
    AssetManager(const AssetManager&) = delete;
    AssetManager& operator=(const AssetManager&) = delete;

    // Asset path
    void SetAssetPath(const std::string& folderName);
    const std::string& GetAssetPath() const { return m_assetPath; }

    // Load
    void LoadTexture(const std::string& key, const std::string& relativePath);
    void LoadSound(const std::string& key, const std::string& relativePath);
    void LoadFont(const std::string& key, const std::string& relativePath, int fontSize = 20);
    void LoadMusic(const std::string& key, const std::string& relativePath);

    // Retrieve
    Texture2D& GetTexture(const std::string& key);
    Sound&     GetSound(const std::string& key);
    Font&      GetFont(const std::string& key);
    Music&     GetMusic(const std::string& key);

    // Query
    bool HasTexture(const std::string& key) const;
    bool HasSound(const std::string& key)   const;
    bool HasFont(const std::string& key)    const;
    bool HasMusic(const std::string& key)   const;

    // Lifecycle
    void Shutdown();

private:
    // Prepends the resolved asset root to a relative path
    std::string ResolvePath(const std::string& relativePath) const;

    std::string m_assetPath; // Set by SearchAssetPath()

    std::unordered_map<std::string, Texture2D> m_textures;
    std::unordered_map<std::string, Sound>     m_sounds;
    std::unordered_map<std::string, Font>      m_fonts;
    std::unordered_map<std::string, Music>     m_music;
};