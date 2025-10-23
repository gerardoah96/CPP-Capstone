#pragma once
#include <SDL2/SDL.h>
#include <string>

// Forward-declare to avoid coupling headers
class Game;

class Renderer {
public:
    // windowW/ windowH are computed from (2 views) * (gridW * tileSize) by caller or via init helper
    Renderer(const std::string& title, int windowW, int windowH, int tileSize);
    ~Renderer();

    // Disallow copy; allow move if desired later
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    bool IsOk() const { return window_ && sdlRenderer_; }

    // Clear the whole window to background
    void BeginFrame();

    // Draw a single Game into a viewport (x,y,w,h in pixels)
    void DrawGameView(const Game& game, const SDL_Rect& viewport);

    // Draw two games side-by-side (split screen). Both viewports are computed from grid/tile.
    void DrawSplit(const Game& left, const Game& right);

    // Present the frame
    void EndFrame();

    // Optional: toggle grid overlay
    void SetGridEnabled(bool enabled) { drawGrid_ = enabled; }

    // Accessors
    SDL_Renderer* Raw() const { return sdlRenderer_; }
    int TileSize() const { return tileSize_; }

private:
    void drawLanes_(const Game& game, const SDL_Rect& vp);
    void drawVehicles_(const Game& game, const SDL_Rect& vp);
    void drawFrog_(const Game& game, const SDL_Rect& vp);
    void drawGridOverlay_(const Game& game, const SDL_Rect& vp);

    // Tile-to-pixel helpers inside a viewport
    inline SDL_Rect tileToPxRect_(float tx, float ty, float tw, float th, const SDL_Rect& vp, int gridH) const;

private:
    SDL_Window*   window_      = nullptr;
    SDL_Renderer* sdlRenderer_ = nullptr;
    int tileSize_ = 32;
    bool drawGrid_ = true;

    // palette
    SDL_Color colBg_         {  8,  8,  8, 255 }; // window background
    SDL_Color colLaneSafe_   { 30,120, 30, 255 }; // safe lane
    SDL_Color colLaneTraffic_{ 45, 45, 45, 255 }; // traffic lane asphalt
    SDL_Color colVehicle_    {200, 40, 40, 255 }; // red vehicles (can vary per lane if you want)
    SDL_Color colGrid_       { 80, 80, 80, 255 }; // grid lines
};
