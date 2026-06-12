#pragma once

#include <raylib.h>
#include <engine/core/resources.hpp>
#include <string>

class SoundSystem {
public:
    SoundSystem() = default;

    // Must be called after resources are loaded
    void Init(Resources& resources);

    // Sfx: fire-and-forget, uses the sfx volume at time of call
    void PlaySfx(const std::string& key);

    // Music: one streaming track at a time; calling PlayMusic while a track is
    // active stops it first
    void PlayMusic(const std::string& key);
    void StopMusic();
    void PauseMusic();
    void ResumeMusic();

    // Volume [0.0, 1.0]; music volume applies immediately to the active track
    void SetMusicVolume(float volume);
    void SetSfxVolume(float volume);
    float GetMusicVolume() const { return m_musicVolume; }
    float GetSfxVolume() const { return m_sfxVolume; }

    // Call once per frame — feeds the active music stream
    void Tick(float dt);

private:
    Resources* m_resources = nullptr;

    // Pointer into Resources; null when nothing is playing
    Music* m_activeMusic = nullptr;
    std::string m_activeKey;

    float m_musicVolume = 1.0f;
    float m_sfxVolume = 1.0f;
};
