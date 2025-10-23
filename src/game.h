#pragma once
#include <deque>
#include <array>
#include <functional>
#include <string>
#include <SDL2/SDL.h>
#include "frog.h"
#include "lane.h"
#include "vehicle.h" // Direction enum

// Discrete one-tile inputs
enum class InputAction { Up, Down, Left, Right };

struct GameSnapshotLane {
    LaneType type;
    Direction dir; // ignored if Safe
    int worldRow;
};

class Game {
public:
    // gridH should be 9 for your design; gridW is how many columns you want to show.
    Game(int gridW, int gridH = 9);

    // Initialize (or reinitialize) with a user-provided seed ("" is allowed).
    // This normalizes to exactly 10 chars per your rule and builds lanes.
    void ResetWithSeed(const std::string& userSeed10, SDL_Color frogColor, int startX);

    // Advance simulation by dt (seconds). Handles lane phase & collisions.
    void Update(float dtSeconds);

    // Apply one-tile input and scoring. Enforces clamping & scroll trigger (4->5).
    // Returns true if the frog's position actually changed.
    bool HandleInput(InputAction a);

    // Game state
    bool IsGameOver() const { return gameOver_; }
    int  Score() const { return frog_.GetScore(); }

    // Rendering helpers
    const Frog& Player() const { return frog_; }
    int GridW() const { return gridW_; }
    int GridH() const { return gridH_; }
    int BottomRowWorld() const { return bottomRowWorld_; }
    int TopRowWorld() const { return topRowWorld_; }

    // Lanes visible now (front = top, back = bottom). Exactly gridH_ lanes.
    const std::deque<Lane>& Lanes() const { return lanes_; }

    // Convenience: iterate visible vehicles tile rects for drawing
    void ForEachVehicle(const std::function<void(const TileRect&)>& fn) const;

    // Expose a compact lane snapshot for UI (types/directions/world rows)
    void SnapshotLanes(std::vector<GameSnapshotLane>& out) const;

    // The normalized 10-char seed and the 64-bit match seed hash
    const std::string& NormalizedSeed() const { return normSeed10_; }
    uint64_t MatchSeed() const { return matchSeed_; }

private:
    // ===== Deterministic lane generation =====
    Lane GenerateLane(int worldRow);
    void EnsurePregen(); // keep next 5 ready
    // Normalize user seed to exactly 10 chars per your spec
    static std::string NormalizeSeed10(std::string s);
    static uint64_t SeedToU64(const std::string& norm10);

    // ===== Camera/scroll rules =====
    // Called after a successful Up/Down move. Handles the 4->5 scroll.
    void ApplyScrollIfNeeded_(int prevY, int newY);
    // Clamp Down so player cant go below current bottom
    int ClampDownTarget_(int desiredY) const;

    // ===== Internals =====
    int gridW_;
    int gridH_;       // should be 9
    Frog frog_ {0,0, SDL_Color{0,255,0,255}}; // default, reset in ResetWithSeed

    bool gameOver_ = false;

    // lane ring buffer (size == gridH_)
    std::deque<Lane> lanes_;
    // pre-generated next lanes (keep 5 ready)
    std::deque<Lane> pregen_;

    // world row indices for current visible window
    int topRowWorld_ = 0;          // world row of lanes_.front()
    int bottomRowWorld_ = 0;       // world row of lanes_.back()
    int lanesAdvanced_ = 0;        // how many times we scrolled

    // seed
    std::string normSeed10_;
    uint64_t matchSeed_ = 0;

    // difficulty ramp (applied to baseSpeed, then clamped to min/max)
    float difficultyAlpha_ = 0.02f; // tweakable growth per scroll

    bool inputLockOnce_ = false;
};
