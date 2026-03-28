#pragma once

#include <states/game_state.hpp>
#include <raylib.h>
#include <systems/render_system.hpp>
#include <systems/world_system.hpp>

class PlayingState : public GameState {
public:
    void OnEnter(Game& game) override;
    void OnExit(Game& game)  override;

    void ProcessInput(Game& game)        override;
    void Update(Game& game, float dt)    override;
    void Draw(Game& game) override;

private:
    bool m_debug = false;
    RenderSystem m_renderSystem;
    WorldSystem m_worldSystem;
};