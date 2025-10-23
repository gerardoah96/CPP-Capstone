#include "game.h"
#include <random>
#include <algorithm>
#include <cmath>

// ---------- Seed helpers ----------
std::string Game::NormalizeSeed10(std::string s) {
    if (s.empty()) {
        std::mt19937_64 rng{std::random_device{}()};
        std::uniform_int_distribution<int> d(0,9);
        std::string out; out.reserve(10);
        for (int i=0;i<10;++i) out.push_back(char('0'+d(rng)));
        return out;
    }
    if (s.size() >= 10) return s.substr(0,10);
    std::string out; out.reserve(10);
    while (out.size() < 10) out += s;
    out.resize(10);
    return out;
}

uint64_t Game::SeedToU64(const std::string& norm10) {
    // FNV-1a 64-bit
    uint64_t h = 1469598103934665603ULL;
    const uint64_t p = 1099511628211ULL;
    for (unsigned char c : norm10) { h ^= c; h *= p; }
    return h;
}

// ---------- Game ----------
Game::Game(int gridW, int gridH)
: gridW_(gridW), gridH_(gridH) {}

void Game::ResetWithSeed(const std::string& userSeed10, SDL_Color frogColor, int startX) {
    normSeed10_ = NormalizeSeed10(userSeed10);
    matchSeed_  = SeedToU64(normSeed10_);

    // Reset frog: start on lane 4 (0-based), x provided by caller
    frog_ = Frog(startX, /*startY*/ 0, frogColor);
    frog_.SetScore(0);
    gameOver_ = false;

    // Build initial lanes: world rows [0..gridH_-1], front=top (gridH_-1), back=bottom (0)
    lanes_.clear();
    pregen_.clear();
    topRowWorld_ = 0;
    bottomRowWorld_ = gridH_ - 1; // we will place so that back is bottom visible row 0 in screen coords

    // We prefer world indices increasing upward. Let's define bottomRowWorld_ = 0 .. topRowWorld_ = 8 initially.
    // So rebuild with bottom=0..top=gridH_-1
    lanes_.clear();
    for (int wr = 0; wr < gridH_; ++wr) {
        // We'll push_front so final order has front=top
        lanes_.push_front(GenerateLane(wr));
    }
    topRowWorld_ = gridH_ - 1;     // world row at lanes_.front()
    bottomRowWorld_ = 0;           // world row at lanes_.back()

    // Pre-generate next 5 lanes above current top
    for (int i = 1; i <= 5; ++i) {
        pregen_.push_back(GenerateLane(topRowWorld_ + i));
    }

    lanesAdvanced_ = 0;
}

// Difficulty multiplier based on progress (scroll count)
static inline float difficultyScaleFrom(int lanesAdvanced, float alpha) {
    // 1 + alpha * lanesAdvanced (can clamp later if desired)
    return std::max(1.0f, 1.0f + alpha * static_cast<float>(lanesAdvanced));
}

void Game::Update(float dtSeconds) {
    if (gameOver_) return;

    float scale = difficultyScaleFrom(lanesAdvanced_, difficultyAlpha_);
    for (auto& ln : lanes_) {
        ln.Update(dtSeconds, scale);
    }

    // Collisions: frog is 1x1 tile rect
    TileRect frogRect{ static_cast<float>(frog_.GetX()),
                       static_cast<float>(frog_.GetY()),
                       1.0f, 1.0f };

   int i = 0;
    for (const auto& ln : lanes_) {
        int logicalY = gridH_ - 1 - i;  // convert topbottom index to logical bottomtop
        if (ln.CollidesAtScreenRow(frogRect, gridW_, logicalY)) {
            gameOver_ = true;
            break;
        }
        ++i;
    }
    if (inputLockOnce_) inputLockOnce_ = false;
}

bool Game::HandleInput(InputAction a) {
    if (gameOver_ || inputLockOnce_) return false;

    int prevX = frog_.GetX();
    int prevY = frog_.GetY();
    bool moved = false;

    switch (a) {
        case InputAction::Up: {
            if (prevY < gridH_ - 1) {     // can go up until top row (8)
                frog_.MoveUp();
                moved = true;
            }
            break;
        }
        case InputAction::Down: {
            if (prevY > 0) {               // can go down until bottom row (0)
                frog_.MoveDown();
                moved = true;
            }
            break;
        }
        case InputAction::Left: {
            if (prevX > 0) { frog_.MoveLeft(); moved = true; }
            break;
        }
        case InputAction::Right: {
            if (prevX < gridW_ - 1) { frog_.MoveRight(); moved = true; }
            break;
        }
    }

    if (moved) {
        ApplyScrollIfNeeded_(prevY, frog_.GetY());
        // Optional: clamp X again (just in case)
        if (frog_.GetX() < 0) frog_.SetPosition(0, frog_.GetY());
        if (frog_.GetX() >= gridW_) frog_.SetPosition(gridW_-1, frog_.GetY());
    }
    return moved;
}

void Game::ApplyScrollIfNeeded_(int prevY, int newY) {
    // logical y (bottom=0)  world row
    const int prevWorld = bottomRowWorld_ + prevY;
    const int newWorld  = bottomRowWorld_ + newY;

    // Trigger when moving UP into the FIRST safe lane of the next block.
    const bool enteringNextBlockFirstSafe =
        ((prevWorld % 7) != 0) && ((newWorld % 7) == 0) && (newWorld > prevWorld);

    if (!enteringNextBlockFirstSafe) return;

    constexpr int kShift = 7;  // drop a whole block: 2 safe + 5 traffic

    // 1) drop 7 lanes from the bottom
    for (int i = 0; i < kShift; ++i) {
        if (!lanes_.empty()) lanes_.pop_back();
    }
    bottomRowWorld_ += kShift;

    // 2) add 7 new lanes at the top
    for (int i = 0; i < kShift; ++i) {
        const int nextWorld = topRowWorld_ + 1;  // always append the next world row
        lanes_.push_front(GenerateLane(nextWorld));
        topRowWorld_ = nextWorld;
    }

    // 3) keep pregeneration healthy (optional if you don't use a pregen deque)
    EnsurePregen(); // if you keep pregen, target >= 14 ahead inside it

    lanesAdvanced_ += kShift;

    // 4) place frog on the SECOND safe lane (row 1).
    // Bottom two lanes are now the new block's safe pair (mod 0 and mod 1).
    frog_.SetPosition(frog_.GetX(), 1);

    // 5) ignore inputs for one frame to avoid consuming a buffered key
    inputLockOnce_ = true;
}

int Game::ClampDownTarget_(int desiredY) const {
    // World rows for visible window are [bottomRowWorld_ .. topRowWorld_]
    // Convert frog's current screen row to world row and ensure desiredY is not below bottom.
    // Since we model frog Y in screen lanes [0..gridH_-1] with bottom == 0,
    // just clamp to 0 on the screen.
    return std::max(0, std::min(gridH_-1, desiredY));
}

void Game::ForEachVehicle(const std::function<void(const TileRect&)>& fn) const {
    int i = 0;
    for (const auto& ln : lanes_) {
        int logicalY = gridH_ - 1 - i;
        ln.ForEachVisibleVehicle(gridW_, logicalY, fn);
        ++i;
    }
}

void Game::SnapshotLanes(std::vector<GameSnapshotLane>& out) const {
    out.clear();
    out.reserve(static_cast<size_t>(gridH_));
    for (const auto& ln : lanes_) {
        out.push_back(GameSnapshotLane{ ln.Type(), ln.Dir(), ln.WorldRow() });
    }
}

void Game::EnsurePregen() {
    while (static_cast<int>(pregen_.size()) < 12) {
        int nextWorldRow = topRowWorld_ + static_cast<int>(pregen_.size()) + 1;
        pregen_.push_back(GenerateLane(nextWorldRow));
    }
}


// Deterministic per-row lane generator from matchSeed_ and worldRow.
// - Guarantees: worldRow == 0 is always Safe (no instant death).
// - Traffic rows: exactly 5 vehicles, lengths  {1,2,3}, gaps  [2..5],
//   min/max/base speeds (tiles/sec), and Left/Right direction.
Lane Game::GenerateLane(int worldRow) {
    // SAFE ZONES: two safe rows per 7-row block.
    // Safe when worldRow % 7 == 0  OR  worldRow % 7 == 1
    int mod = worldRow % 7;
    if (mod == 0 || mod == 1) {
        LaneConfig cfg;
        cfg.type = LaneType::Safe;
        cfg.dir  = Direction::Right; // ignored
        for (auto& s : cfg.pattern) { s.lengthTiles = 1; s.gapTiles = 2; s.offset = 0.f; }
        return Lane(worldRow, cfg);
    }

    // Deterministic RNG from (matchSeed_, worldRow)
    auto splitmix64 = [](uint64_t& x) {
        uint64_t z = (x += 0x9E3779B97F4A7C15ULL);
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
        return z ^ (z >> 31);
    };
    uint64_t sm = matchSeed_ ^ (0xD6E8FEB86659FD93ULL * static_cast<uint64_t>(worldRow));
    auto nextU = [&]() -> uint64_t { return splitmix64(sm); };
    auto nextF = [&](float lo, float hi) {
        double r = (nextU() >> 11) * (1.0 / 9007199254740992.0);
        return static_cast<float>(lo + (hi - lo) * r);
    };
    auto nextI = [&](int lo, int hi) { return lo + static_cast<int>(nextU() % (static_cast<uint64_t>(hi - lo + 1))); };

    // Alternate directions across traffic rows within the 7-row block
    int inBlock = worldRow % 7;      // 0..6 (0/1 are Safe, 2..6 are Traffic)
    int blockId = worldRow / 7;
    Direction dir = ((blockId + inBlock) % 2 == 0) ? Direction::Left : Direction::Right;


    // Speeds (tiles/sec)
    float minS = nextF(1.5f, 3.0f);
    float maxS = std::min(4.5f, minS + nextF(0.5f, 2.0f));
    float base = nextF(minS, maxS);

    // 5 vehicles, lengths 1/2/3 (biased to 2/3), gaps 2..5
    std::array<int,5> lengths{};
    std::array<int,5> gaps{};
    for (int i=0;i<5;++i) {
        int r = nextI(0, 99);
        lengths[i] = (r < 20) ? 1 : (r < 60 ? 2 : 3);
        gaps[i]    = nextI(2, 5);
    }

    return Lane(worldRow, LaneType::Traffic, dir, minS, maxS, base, lengths, gaps);
}
