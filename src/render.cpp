#include "render.h"
#include "game.h"
#include "frog.h"
#include "lane.h"
#include <vector>
#include <algorithm>

Renderer::Renderer(const std::string& title, int windowW, int windowH, int tileSize)
: tileSize_(tileSize)
{
    SDL_Init(SDL_INIT_VIDEO);
    window_ = SDL_CreateWindow(
        title.c_str(),
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        windowW, windowH,
        SDL_WINDOW_SHOWN
    );
    sdlRenderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
}

Renderer::~Renderer() {
    if (sdlRenderer_) { SDL_DestroyRenderer(sdlRenderer_); sdlRenderer_ = nullptr; }
    if (window_)      { SDL_DestroyWindow(window_);         window_      = nullptr; }
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

void Renderer::BeginFrame() {
    SDL_SetRenderDrawColor(sdlRenderer_, colBg_.r, colBg_.g, colBg_.b, colBg_.a);
    SDL_RenderClear(sdlRenderer_);
}

void Renderer::EndFrame() {
    SDL_RenderPresent(sdlRenderer_);
}

void Renderer::DrawSplit(const Game& left, const Game& right) {
    const int viewW = left.GridW() * tileSize_;
    const int viewH = left.GridH() * tileSize_;
    SDL_Rect vpLeft{ 0, 0, viewW, viewH };
    SDL_Rect vpRight{ viewW, 0, viewW, viewH };

    DrawGameView(left, vpLeft);
    DrawGameView(right, vpRight);

    // vertical divider between views
    // vertical gutter = at least one full tile (use 2 tiles if you want extra margin)
    int gutterW = std::max( tileSize_, tileSize_ );   // change to 2*tileSize_ if you prefer
    int gutterX = vpLeft.w - gutterW/2;

    SDL_SetRenderDrawColor(sdlRenderer_, 12, 12, 12, 255);
    SDL_Rect mid { gutterX, 0, gutterW, vpLeft.h };
    SDL_RenderFillRect(sdlRenderer_, &mid);
}

void Renderer::DrawGameView(const Game& game, const SDL_Rect& vp) {
    // Fill background (just in case)
    SDL_SetRenderDrawColor(sdlRenderer_, colBg_.r, colBg_.g, colBg_.b, colBg_.a);
    SDL_RenderFillRect(sdlRenderer_, &vp);

    drawLanes_(game, vp);
    drawVehicles_(game, vp);
    drawFrog_(game, vp);
    if (drawGrid_) drawGridOverlay_(game, vp);
}

SDL_Rect Renderer::tileToPxRect_(float tx, float ty, float tw, float th,
                                 const SDL_Rect& vp, int gridH) const {
    // Flip Y: logical y=0 (bottom) -> pixel row (gridH-1)
    float flippedY = static_cast<float>(gridH) - (ty + th);
    SDL_Rect r;
    r.x = vp.x + static_cast<int>(tx * tileSize_);
    r.y = vp.y + static_cast<int>(flippedY * tileSize_);
    r.w = static_cast<int>(tw * tileSize_);
    r.h = static_cast<int>(th * tileSize_);
    return r;
}


void Renderer::drawLanes_(const Game& game, const SDL_Rect& vp) {
    // Query lane snapshot to know lane types for background
    std::vector<GameSnapshotLane> lanes;
    game.SnapshotLanes(lanes);

    const int rows = game.GridH();
    // lanes vector is ordered front=top to back=bottom. We'll draw from top (y=0) to bottom.
    for (int logicalY = 0; logicalY < rows; ++logicalY) {
        const GameSnapshotLane& ln = lanes[ static_cast<size_t>(rows - 1 - logicalY) ];
        SDL_Color c = (ln.type == LaneType::Safe) ? colLaneSafe_ : colLaneTraffic_;
        SDL_SetRenderDrawColor(sdlRenderer_, c.r, c.g, c.b, c.a);
        SDL_Rect rect = tileToPxRect_(0.f, static_cast<float>(logicalY),
                                    static_cast<float>(game.GridW()), 1.f, vp, rows);
        SDL_RenderFillRect(sdlRenderer_, &rect);
    }
}

void Renderer::drawVehicles_(const Game& game, const SDL_Rect& vp) {
    SDL_SetRenderDrawColor(sdlRenderer_, colVehicle_.r, colVehicle_.g, colVehicle_.b, colVehicle_.a);
    game.ForEachVehicle([&](const TileRect& trect){
        SDL_Rect r = tileToPxRect_(trect.x, trect.y, trect.w, trect.h, vp, game.GridH());
        SDL_RenderFillRect(sdlRenderer_, &r);
    });
}

void Renderer::drawFrog_(const Game& game, const SDL_Rect& vp) {
    const Frog& f = game.Player();
    SDL_Color fc = f.GetColor();
    SDL_SetRenderDrawColor(sdlRenderer_, fc.r, fc.g, fc.b, fc.a);
    // Frog dimensions are 1x1 tile; convert from tile coords to pixels
    SDL_Rect r = tileToPxRect_(static_cast<float>(f.GetX()), static_cast<float>(f.GetY()), 1.f, 1.f, vp, game.GridH());
    SDL_RenderFillRect(sdlRenderer_, &r);
}

void Renderer::drawGridOverlay_(const Game& game, const SDL_Rect& vp) {
    SDL_SetRenderDrawColor(sdlRenderer_, colGrid_.r, colGrid_.g, colGrid_.b, colGrid_.a);

    // Vertical lines
    for (int x = 0; x <= game.GridW(); ++x) {
        int px = vp.x + x * tileSize_;
        SDL_RenderDrawLine(sdlRenderer_, px, vp.y, px, vp.y + vp.h);
    }
    // Horizontal lines
    for (int y = 0; y <= game.GridH(); ++y) {
        int py = vp.y + y * tileSize_;
        SDL_RenderDrawLine(sdlRenderer_, vp.x, py, vp.x + vp.w, py);
    }
}
