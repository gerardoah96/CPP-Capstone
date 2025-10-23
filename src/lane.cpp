#include "lane.h"
#include <algorithm>

static inline float clampf(float v, float lo, float hi) {
    return std::max(lo, std::min(v, hi));
}

Lane::Lane(int worldRowIndex,
           LaneType type,
           Direction dir,
           float minSpeedTilesSec,
           float maxSpeedTilesSec,
           float baseSpeedTilesSec,
           const std::array<int,5>& lengths,
           const std::array<int,5>& gaps)
: worldRowIndex_(worldRowIndex),
  type_(type),
  dir_(dir),
  minSpeed_(minSpeedTilesSec),
  maxSpeed_(maxSpeedTilesSec),
  baseSpeed_(baseSpeedTilesSec) {

    for (std::size_t i = 0; i < slots_.size(); ++i) {
        slots_[i].lengthTiles = lengths[i];
        slots_[i].gapTiles    = gaps[i];
        slots_[i].offset      = 0.f; // computed next
    }
    buildPatternOffsets_();
}

Lane::Lane(int worldRowIndex, const LaneConfig& cfg)
: worldRowIndex_(worldRowIndex),
  type_(cfg.type),
  dir_(cfg.dir),
  minSpeed_(cfg.minSpeedTilesSec),
  maxSpeed_(cfg.maxSpeedTilesSec),
  baseSpeed_(cfg.baseSpeedTilesSec),
  slots_(cfg.pattern) {
    buildPatternOffsets_();
}

void Lane::buildPatternOffsets_() {
    loopLenTiles_ = 0.f;
    for (std::size_t i = 0; i < slots_.size(); ++i) {
        slots_[i].offset = loopLenTiles_;
        loopLenTiles_ += static_cast<float>(slots_[i].lengthTiles + slots_[i].gapTiles);
    }
    if (loopLenTiles_ <= 0.f) loopLenTiles_ = 1.f; // guard
    phase_ = std::fmod(phase_, loopLenTiles_);
}

float Lane::CurrentSpeed(float difficultyScale) const {
    if (type_ == LaneType::Safe) return 0.f;
    float scaled = baseSpeed_ * std::max(1.f, difficultyScale);
    return clampf(scaled, minSpeed_, maxSpeed_);
}

void Lane::Update(float dtSeconds, float difficultyScale) {
    if (type_ == LaneType::Safe) return;
    const float eff = CurrentSpeed(difficultyScale);
    phase_ = std::fmod(phase_ + eff * dtSeconds, loopLenTiles_);
    if (phase_ < 0.f) phase_ += loopLenTiles_;
}

// Convert a slot's loop offset to the vehicle's LEFT x coordinate in tile units.
// We arrange vehicles on an infinite line spaced by 'offset', then scroll with 'phase_'.
// Right-moving lanes advance x as phase grows; left-moving recede.
float Lane::slotX_(float slotOffset) const {
    // Normalize slot position along the loop
    float t = std::fmod(slotOffset + phase_, loopLenTiles_);
    if (t < 0.f) t += loopLenTiles_;

    // We map the loop directly onto world x:
    // - For Right: x = -t  (vehicles start offscreen left and move right as phase increases)
    // - For Left : x = t   (we'll subtract length when building rect)
    return (dir_ == Direction::Right) ? -t : t;
}

void Lane::ForEachVisibleVehicle(int gridW, int screenRowY,
                                 const std::function<void(const TileRect&)>& fn) const
{
    if (type_ == LaneType::Safe) return;

    for (const auto& s : slots_) {
        // phase along the repeating loop [0, L)
        float L = loopLenTiles_;
        float phase = std::fmod(phase_, L);
        if (phase < 0.f) phase += L;

        float w = static_cast<float>(s.lengthTiles);
        float x;

        if (dir_ == Direction::Right) {
            // Start fully off-screen left, move right as phase increases
            // x = -w + (phase - offset)
            x = -w + (phase - s.offset);
        } else {
            // Start fully off-screen right, move left as phase increases
            // x = gridW + (offset - phase)
            x = static_cast<float>(gridW) + (s.offset - phase);
        }

        // Cull against [0, gridW)
        if (x + w <= 0.f || x >= static_cast<float>(gridW)) continue;

        TileRect rect{ x, static_cast<float>(screenRowY), w, 1.0f };
        fn(rect);
    }
}

bool Lane::CollidesAtScreenRow(const TileRect& player, int gridW, int screenRowY) const {
    if (type_ == LaneType::Safe) return false;

    bool hit = false;
    ForEachVisibleVehicle(gridW, screenRowY, [&](const TileRect& v){
        bool overlapX = (player.x < v.x + v.w) && (player.x + player.w > v.x);
        bool overlapY = (player.y < v.y + v.h) && (player.y + player.h > v.y);
        if (overlapX && overlapY) hit = true;
    });
    return hit;
}

