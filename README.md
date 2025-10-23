# Frogger Concurrent — Two Player Split-Screen Version

A modern C++ recreation of the classic **Frogger**, built from scratch with SDL2 and multithreading.  
Each player runs their own fully-independent simulation thread (Frog + world), and both share a deterministic map generated from the same seed.

---

## 🎮 Features

- **Two concurrent games** running in parallel threads inside one process.
- **Split-screen renderer** with independent lanes, vehicles, and scoring.
- **Deterministic seed map** — identical lane layouts for both players.
- **Procedural lane generation:** repeating blocks of traffic and safe zones.
- **Dynamic difficulty scaling:** traffic speed increases with distance.
- **Chunk-based world streaming:** old lanes are popped, new ones generated seamlessly.
- **Collision detection** for vehicle-frog overlap.
- **Score tracking:** +1 per upward hop, −1 per downward hop.
- **Safe zones:** two-lane safety pads every 7 lanes.
- **Smart resource management:** all dynamic allocations use RAII and `unique_ptr`.
- **Multithreading + synchronization:**  
  - Each game runs in its own simulation thread.  
  - Queued inputs are thread-safe (`TSQueue`).
- **Seed system:** Enter a 10-digit seed (or blank for random).  
  - Same seed → same map across both players.

---

## 🕹️ Controls

| Player | Move Up | Move Down | Move Left | Move Right |
|:-------|:---------|:-----------|:------------|:------------|
| Green (Player 1) | **W** | **S** | **A** | **D** |
| Blue (Player 2) | **↑** | **↓** | **←** | **→** |

Other keys:
- **R** → Restart after game-over  
- **ESC** → Quit

---

## 🧠 Technical Summary (Rubric Coverage)

### Loops, Functions, I/O
- Structured `Game`, `Lane`, and `Render` loops.
- Reads/writes seed strings from user input.
- Uses vectors/deques for lane management.
- Accepts and processes keyboard input.

### Object-Oriented Programming
- Classes: `Frog`, `Vehicle`, `Lane`, `Game`, `Renderer`.
- Clear access specifiers and encapsulation.
- Constructors use **member initialization lists**.
- Inheritance ready (e.g. future extension for different vehicle types).
- Overloaded / templated queue (`TSQueue<T>`).

### Memory Management
- Uses **RAII**; no manual new/delete.
- Smart pointers (`unique_ptr` for SDL window/renderer wrappers if added).
- Move semantics used when pushing lanes into `deque`.
- Proper destructors to free SDL resources.

### Concurrency
- Two **simulation threads**, one per player.
- **Thread-safe queue** for inputs (mutex-protected).
- **Atomic flags** to signal stop conditions.
- Input queues cleared safely on restart.

---

## 🧱 Build & Run

### Requirements
- **CMake 3.10+**
- **SDL2** installed on your system

### Build commands
```bash
mkdir build
cd build
cmake ..
make
./frogger
```

If SDL2 isn’t found, install it via:
```bash
brew install sdl2
```

### Run
On launch, you’ll be prompted for a seed:
- Enter 10-digit seed (deterministic)
- Or press **Enter** for random map generation

---

## 🪄 Seed Rules

| Case | Behavior |
|------|-----------|
| Blank | Generate random 10-digit numeric seed |
| < 10 chars | Repeats until 10 digits |
| > 10 chars | Truncated to first 10 |
| Same seed | Ensures identical map generation for both players |

---

## 🧩 Code Structure

```
src/
 ├── main.cpp        # Thread orchestration, event loop
 ├── game.cpp/.h     # Core game logic & world updates
 ├── render.cpp/.h   # SDL2 drawing (split-screen)
 ├── frog.cpp/.h     # Player logic
 ├── vehicle.cpp/.h  # Vehicle logic
 ├── lane.cpp/.h     # Lanes and pattern generation
assets/
 └── Frogger.gif     # Gameplay preview
CMakeLists.txt
README.md
```

---

## 🧵 Example Gameplay Preview

![Game Preview](assets/Frogger.gif)

---

## 💡 Future Enhancements
- Add scoring UI text using SDL_ttf.
- Add optional sound effects.
- Support for local multiplayer input remapping.
- Extend to network multiplayer using sockets.

---

## 📜 License
MIT License © 2025 Gerardo Arguello Haces
