#include <SDL2/SDL.h>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <random>

#include "game.h"
#include "render.h"

template <typename T>
class TSQueue {
public:
    void push(const T& v) { std::lock_guard<std::mutex> lock(m_); q_.push(v); }
    bool pop(T& out)      { std::lock_guard<std::mutex> lock(m_); if(q_.empty()) return false; out=q_.front(); q_.pop(); return true; }
    void clear()          { std::lock_guard<std::mutex> lock(m_); std::queue<T> empty; std::swap(q_, empty); }
private:
    std::mutex m_;
    std::queue<T> q_;
};

static void SimLoop(Game& game,
                    TSQueue<InputAction>& inQ,
                    std::atomic<bool>& stopFlag)
{
    using clock = std::chrono::steady_clock;
    const double dt = 1.0 / 60.0;
    auto next = clock::now();

    while (!stopFlag.load()) {
        InputAction act;
        while (inQ.pop(act)) {
            game.HandleInput(act);
        }

        game.Update(static_cast<float>(dt));
        if (game.IsGameOver()) break;

        next += std::chrono::microseconds(static_cast<int>(dt * 1'000'000));
        std::this_thread::sleep_until(next);
    }
}

enum class AppState { Playing, GameOver };

static std::string normalizeSeed(std::string s) {
    if (s.empty()) {
        std::mt19937_64 rng{std::random_device{}()};
        std::uniform_int_distribution<int> d(0, 9);
        s.reserve(10);
        for (int i = 0; i < 10; ++i) s.push_back('0' + d(rng));
        return s;
    }
    if (s.size() > 10) return s.substr(0, 10);
    while (s.size() < 10) s += s;
    return s.substr(0, 10);
}

static void ResetBoth(Game& gameA, Game& gameB, const std::string& seed, int gridW) {
    SDL_Color green{0,255,0,255}, blue{0,0,255,255};
    int startX = gridW / 2;
    gameA.ResetWithSeed(seed, green, startX);
    gameB.ResetWithSeed(seed, blue,  startX);
}

int main(int, char**) {
    const int gridW = 15, gridH = 9, tile = 32;
    std::cout << "Enter 10-char seed (any length; empty for random): ";
    std::string userSeed;
    std::getline(std::cin, userSeed);
    std::string normalizedSeed = normalizeSeed(userSeed);
    std::cout << "Using seed: " << normalizedSeed << "\n";

    Game gameA(gridW, gridH);
    Game gameB(gridW, gridH);
    ResetBoth(gameA, gameB, normalizedSeed, gridW);

    const int windowW = 2 * gridW * tile;
    const int windowH = gridH * tile;
    Renderer renderer("Frogger Split", windowW, windowH, tile);
    if (!renderer.IsOk()) {
        std::cerr << "SDL init failed.\n";
        return 1;
    }

    TSQueue<InputAction> inA, inB;
    std::atomic<bool> stopA{false}, stopB{false};
    std::thread tA, tB;

    auto startSession = [&]() {
        stopA.store(false); stopB.store(false);
        tA = std::thread(SimLoop, std::ref(gameA), std::ref(inA), std::ref(stopA));
        tB = std::thread(SimLoop, std::ref(gameB), std::ref(inB), std::ref(stopB));
    };
    auto endSession = [&]() {
        stopA.store(true); stopB.store(true);
        if (tA.joinable()) tA.join();
        if (tB.joinable()) tB.join();
    };

    startSession();

    AppState state = AppState::Playing;
    SDL_Rect playAgainBtn{ windowW/2 - 120, windowH/2 - 30, 240, 60 };

    bool quit = false;
    while (!quit) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) quit = true;
            else if (e.type == SDL_KEYDOWN && e.key.repeat == 0) {
                if (e.key.keysym.sym == SDLK_ESCAPE) quit = true;
                else if (state == AppState::Playing) {
                    if (e.key.keysym.sym == SDLK_w) inA.push(InputAction::Up);
                    else if (e.key.keysym.sym == SDLK_s) inA.push(InputAction::Down);
                    else if (e.key.keysym.sym == SDLK_a) inA.push(InputAction::Left);
                    else if (e.key.keysym.sym == SDLK_d) inA.push(InputAction::Right);
                    else if (e.key.keysym.sym == SDLK_UP)    inB.push(InputAction::Up);
                    else if (e.key.keysym.sym == SDLK_DOWN)  inB.push(InputAction::Down);
                    else if (e.key.keysym.sym == SDLK_LEFT)  inB.push(InputAction::Left);
                    else if (e.key.keysym.sym == SDLK_RIGHT) inB.push(InputAction::Right);
                } else if (state == AppState::GameOver) {
                    if (e.key.keysym.sym == SDLK_r) {
                        endSession();
                        inA.clear();
                        inB.clear();
                        ResetBoth(gameA, gameB, normalizedSeed, gridW);
                        startSession();
                        state = AppState::Playing;
                    }
                }
            } else if (e.type == SDL_MOUSEBUTTONDOWN && state == AppState::GameOver) {
                int mx = e.button.x, my = e.button.y;
                if (mx >= playAgainBtn.x && mx <= playAgainBtn.x + playAgainBtn.w &&
                    my >= playAgainBtn.y && my <= playAgainBtn.y + playAgainBtn.h) {
                    endSession();
                    inA.clear();
                    inB.clear();
                    ResetBoth(gameA, gameB, normalizedSeed, gridW);
                    startSession();
                    state = AppState::Playing;
                }
            }
        }

        if (state == AppState::Playing) {
            if (gameA.IsGameOver() && gameB.IsGameOver()) {
                state = AppState::GameOver;
                int sA = gameA.Score(), sB = gameB.Score();
                std::string winner = (sA > sB) ? "Player 1 wins" : (sB > sA) ? "Player 2 wins" : "Tie";
                std::cout << "Scores  P1:" << sA << "  P2:" << sB << "  -> " << winner << "\n";
            }
        }

        renderer.BeginFrame();
        renderer.DrawSplit(gameA, gameB);

        if (state == AppState::GameOver) {
            SDL_SetRenderDrawColor(renderer.Raw(), 0, 0, 0, 160);
            SDL_Rect overlay{0, 0, windowW, windowH};
            SDL_RenderFillRect(renderer.Raw(), &overlay);
            SDL_SetRenderDrawColor(renderer.Raw(), 60, 160, 60, 255);
            SDL_RenderFillRect(renderer.Raw(), &playAgainBtn);
            SDL_SetRenderDrawColor(renderer.Raw(), 10, 40, 10, 255);
            SDL_RenderDrawRect(renderer.Raw(), &playAgainBtn);
        }

        renderer.EndFrame();
        SDL_Delay(1);
    }

    endSession();
    return 0;
}
