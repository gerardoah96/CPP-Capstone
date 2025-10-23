#include "vehicle.h"

Vehicle::Vehicle(int startX, int startY, int length, float speed, Direction dir, SDL_Color color)
    : x_(startX), y_(startY), length_(length), speed_(speed), dir_(dir), color_(color) {}

void Vehicle::Update(float deltaTime) {
    // Move horizontally depending on direction
    if (dir_ == Direction::Right) {
        x_ += speed_ * deltaTime;
    } else {
        x_ -= speed_ * deltaTime;
    }
}

SDL_Rect Vehicle::GetRect(int tileSize) const {
    SDL_Rect rect;
    rect.x = static_cast<int>(x_ * tileSize);
    rect.y = y_ * tileSize;
    rect.w = length_ * tileSize;
    rect.h = tileSize;
    return rect;
}

bool Vehicle::IsOffScreen(int boardWidth) const {
    // If completely off the screen horizontally
    if (dir_ == Direction::Right) {
        return x_ > boardWidth;
    } else {
        return (x_ + length_) < 0;
    }
}
