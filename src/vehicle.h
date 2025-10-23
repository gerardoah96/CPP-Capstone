#pragma once
#include <SDL2/SDL.h>

enum class Direction { Left, Right };

class Vehicle {
public:
    // Constructor
    Vehicle(int startX, int startY, int length, float speed, Direction dir, SDL_Color color);

    // Update position based on speed and delta time
    void Update(float deltaTime);

    // Getters
    int GetX() const { return x_; }
    int GetY() const { return y_; }
    int GetLength() const { return length_; }
    float GetSpeed() const { return speed_; }
    Direction GetDirection() const { return dir_; }
    SDL_Color GetColor() const { return color_; }

    // Rendering helper
    SDL_Rect GetRect(int tileSize) const;

    // Checks if the vehicle is off-screen (for deletion)
    bool IsOffScreen(int boardWidth) const;

private:
    float x_;            // horizontal position (float for smooth movement)
    int y_;              // vertical position (lane index)
    int length_;         // in tiles
    float speed_;        // tiles per second
    Direction dir_;
    SDL_Color color_;
};
