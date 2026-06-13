#pragma once

// Forward declare Game so states can trigger transitions
class Game;

class GameState {
public:
    virtual ~GameState() = default;

    virtual void OnEnter(Game& game) {}
    virtual void OnExit(Game& game) {}
    // Called when this state becomes active again after an overlay above it is popped (no OnEnter
    // runs on resume). Lets a resumed state pick up settings changed while the overlay was open.
    virtual void OnResume(Game& game) {}

    virtual void ProcessInput(Game& game, float dt) = 0;
    virtual void Update(Game& game, float dt) = 0;
    virtual void Draw(Game& game) = 0;
};