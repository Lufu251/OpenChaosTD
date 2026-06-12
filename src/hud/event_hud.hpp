#pragma once

#include <hud/hud.hpp>
#include <string>
#include <vector>

class EventHUD : public HUD {
public:
    // Add a message; adding the same message as the newest entry resets its timer
    void Add(const std::string& message, float duration = 3.0f);
    void Update(float dt);
    void Draw();

private:
    struct Entry {
        std::string m_message;
        float m_timeLeft;
    };

    std::vector<Entry> m_entries;
};
