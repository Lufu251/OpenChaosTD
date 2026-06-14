#include <engine/systems/sound_system.hpp>
#include <engine/core/resources.hpp>

void SoundSystem::Init(Resources& resources) {
    m_resources = &resources;
}

// Sfx
void SoundSystem::PlaySfx(const std::string& key) {
    // No-op for an unset key (e.g. a tower with no attack sound) or a key that
    // never loaded, so a missing/typo'd sound silently does nothing instead of throwing.
    if (key.empty() || !m_resources->HasSound(key)) return;

    Sound& sfx = m_resources->GetSound(key);
    SetSoundVolume(sfx, m_sfxVolume);
    ::PlaySound(sfx);
}

// Music
void SoundSystem::PlayMusic(const std::string& key) {
    // No-op for an unset key or one that never loaded (mirrors PlaySfx), so a missing/typo'd or
    // failed-to-decode music key silently does nothing instead of dereferencing a missing entry.
    if (key.empty() || !m_resources->HasMusic(key)) return;

    if (m_activeMusic)
        StopMusicStream(*m_activeMusic);

    m_activeMusic = &m_resources->GetMusic(key);
    m_activeKey = key;
    ::SetMusicVolume(*m_activeMusic, m_musicVolume);
    PlayMusicStream(*m_activeMusic);
}

void SoundSystem::StopMusic() {
    if (!m_activeMusic) return;
    StopMusicStream(*m_activeMusic);
    m_activeMusic = nullptr;
    m_activeKey.clear();
}

// Volume
void SoundSystem::SetMusicVolume(float volume) {
    m_musicVolume = volume;
    if (m_activeMusic) ::SetMusicVolume(*m_activeMusic, m_musicVolume);
}

void SoundSystem::SetSfxVolume(float volume) {
    m_sfxVolume = volume;
}

// Tick
void SoundSystem::Tick(float dt) {
    (void)dt;
    if (!m_activeMusic) return;
    // Re-resolve the stream from Resources by key each tick. If the datapack unloaded it (e.g. on a
    // datapack switch) the cached pointer would dangle, so drop it instead of updating freed memory;
    // the map is node-based, so re-fetching also survives the key being unloaded and reloaded.
    if (!m_resources->HasMusic(m_activeKey)) {
        m_activeMusic = nullptr;
        m_activeKey.clear();
        return;
    }
    m_activeMusic = &m_resources->GetMusic(m_activeKey);
    UpdateMusicStream(*m_activeMusic);
}
