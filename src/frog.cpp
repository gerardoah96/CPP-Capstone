#include "frog.h"

Frog::Frog(int startX, int startY, SDL_Color color)
    : x_(startX), y_(startY), color_(color) {}

void Frog::MoveUp() {
    y_ += 1;        // up moves toward top (bigger y)
    score_ += 1;
}
void Frog::MoveDown() {
    if (y_ == 0) return;  // bottom clamp here (or in Game)
    y_ -= 1;
    score_ -= 1;
}
void Frog::MoveLeft()  { x_ -= 1; }
void Frog::MoveRight() { x_ += 1; }

void Frog::SetPosition(int x, int y) {
    x_ = x;
    y_ = y;
}

void Frog::SetScore(int score) { score_ = score; }

SDL_Rect Frog::GetRect(int tileSize) const {
    SDL_Rect rect;
    rect.x = x_ * tileSize;
    rect.y = y_ * tileSize;
    rect.w = width_ * tileSize;
    rect.h = height_ * tileSize;
    return rect;
}
