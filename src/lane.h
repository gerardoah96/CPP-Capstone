#pragma once
#include <array>
#include <functional>
#include <cstddef>
#include <cmath>
#include "vehicle.h"  // for Direction

// Visible vs traffic lanes
enum class LaneType { Safe, Traffic };

// Simple tile-space rectangle (no pixels); w/h can be non-integers for vehicles in motion
struct TileRect {
    float x, y, w, h;
};

// One of the five repeating vehicles on the loop
struct VehicleSlot {
    int   lengthTiles;  // 1, 2, or 3
    int   gapTiles;     // 2..5 (distance after this vehicle to next)
    float offset;       // cumulative start offset along loop (computed)
};

// Configuration for a single lane
struct LaneConfig {
    LaneType type          = LaneType::Safe;
    Direction dir          = Direction::Right;  // ignored for Safe
    float minSpeedTilesSec = 0.f;
    float maxSpeedTilesSec = 0.f;
    float baseSpeedTilesSec= 0.f;               // pre-ramp baseline
    std::array<VehicleSlot, 5> pattern{};       // exactly 5 vehicles
};

class Lane {
public:
    // Construct a lane on world row 'worldRowIndex' with the fixed 5-vehicle pattern.
    // 'lengths' and 'gaps' are 5-element arrays; gaps should be in [2,5].
    Lane(int worldRowIndex,
         LaneType type,
         Direction dir,
         float minSpeedTilesSec,
         float maxSpeedTilesSec,
         float baseSpeedTilesSec,
         const std::array<int,5>& lengths,
         const std::array<int,5>& gaps);

    // Construct directly from a prebuilt LaneConfig (pattern offsets will be normalized).
    explicit Lane(int worldRowIndex, const LaneConfig& cfg);

    // Advance the lane's phase by dt (seconds) using clamped speed * difficultyScale (>=1).
    void Update(float dtSeconds, float difficultyScale);

    // Current clamped speed (tiles/sec)
    float CurrentSpeed(float difficultyScale) const;

    // screenRowY: the row index in [0..gridH-1] where this lane is currently drawn
    void ForEachVisibleVehicle(int gridW, int screenRowY,
                            const std::function<void(const TileRect&)>& fn) const;

    // 'player' is in screen tile coords; compare against this lane at 'screenRowY'
    bool CollidesAtScreenRow(const TileRect& player, int gridW, int screenRowY) const;


    // Accessors
    LaneType Type() const { return type_; }
    Direction Dir() const { return dir_; }
    int WorldRow() const { return worldRowIndex_; }
    void SetWorldRow(int r) { worldRowIndex_ = r; }

    // Loop length in tiles (sum of all (length + gap))
    float LoopLenTiles() const { return loopLenTiles_; }

private:
    void buildPatternOffsets_(); // computes offsets & loop length from slots

    // Map a slot's loop offset -> tile x coordinate for current phase & direction
    float slotX_(float slotOffset) const;

private:
    int worldRowIndex_ = 0;

    LaneType type_     = LaneType::Safe;
    Direction dir_     = Direction::Right;

    float minSpeed_    = 0.f;   // tiles/sec
    float maxSpeed_    = 0.f;   // tiles/sec
    float baseSpeed_   = 0.f;   // tiles/sec

    std::array<VehicleSlot,5> slots_{};
    float loopLenTiles_ = 0.f;  // £(length + gap)
    float phase_        = 0.f;  // 0..loopLenTiles, advances with Update()
};
