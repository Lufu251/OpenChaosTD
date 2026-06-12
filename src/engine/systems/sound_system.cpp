#include <engine/systems/sound_system.hpp>

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

void SoundSystem::PauseMusic() {
    if (m_activeMusic) PauseMusicStream(*m_activeMusic);
}

void SoundSystem::ResumeMusic() {
    if (m_activeMusic) ResumeMusicStream(*m_activeMusic);
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
    if (m_activeMusic) UpdateMusicStream(*m_activeMusic);
}
