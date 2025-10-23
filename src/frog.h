#pragma once
#include <SDL2/SDL.h>   // for color and rendering rectangle

class Frog {
public:
    // Constructor
    Frog(int startX, int startY, SDL_Color color);

    // Movement controls
    void MoveUp();
    void MoveDown();
    void MoveLeft();
    void MoveRight();

    // Getters
    int GetX() const { return x_; }
    int GetY() const { return y_; }
    int GetWidth() const { return width_; }
    int GetHeight() const { return height_; }
    SDL_Color GetColor() const { return color_; }
    int GetScore() const { return score_; }

    // Setters
    void SetPosition(int x, int y);
    void SetScore(int score);

    // Rendering support
    SDL_Rect GetRect(int tileSize) const;  // Returns pixel rectangle for rendering

private:
    int x_;
    int y_;
    int width_ = 1;      // tile width
    int height_ = 1;     // tile height
    int score_ = 0;
    SDL_Color color_;
};
