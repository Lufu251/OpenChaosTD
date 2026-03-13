#pragma once

// Forward declare Game so states can trigger transitions
class Game;

class GameState {
public:
    virtual ~GameState() = default;

    virtual void OnEnter(Game& game) {}
    virtual void OnExit(Game& game) {}

    virtual void ProcessInput(Game& game) = 0;
    virtual void Update(Game& game, float dt) = 0;
    virtual void Draw(Game& game) = 0;
};