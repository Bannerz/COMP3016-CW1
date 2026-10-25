# Protolaunch

Protolaunch is a slingshot upgrade game being developed in C++ for COMP3016.
Drag an object back, release it to launch, collect coins, and spend them on
upgrades to beat your previous score.

## Planned gameplay

**Launch → collect coins → upgrade → beat your score → repeat.**

- Aim and launch the object using a mouse-driven slingshot.
- Collect coins during each attempt.
- Spend coins on upgrades between launches.
- Use those upgrades to improve future attempts and set a higher score.
- Later upgrades will introduce keyboard controls during flight.

## Planned controls

| Input | Action | Availability |
| --- | --- | --- |
| Click and drag the object | Pull back and aim the slingshot | Planned |
| Release the mouse button | Launch the object | Planned |
| Arrow keys | Control the object during flight | Planned for future upgrades; exact actions to be decided |

## Built with

C++20, SDL 3.2.28, Box2D 3.1.1, CMake, and VS Code.

## Build and run

CMake downloads SDL3 and Box2D automatically. The first build requires internet access.
Run these commands from the project folder.

### macOS

Requires Xcode Command Line Tools, Git, CMake 3.24+, and Ninja.

```sh
cmake --preset macos-debug
cmake --build --preset macos-debug
./build/macos-debug/game
```

### Windows

Requires Git, CMake 3.24+, and Visual Studio 2022 Build Tools with the
**Desktop development with C++** workload and a Windows SDK.

```powershell
cmake --preset windows
cmake --build --preset windows-debug
.\build\windows\Debug\game.exe
```

### VS Code

Install the recommended extensions, select your platform's CMake preset, and
configure the project. Choose the matching **Game** debugger configuration and
press **F5**.

## Code structure

- `main.cpp` — SDL setup, input, the game loop, and shutdown.
- `CoinManager.h` / `.cpp` — coin chunks, pickups, drawing, and run resets.
- `Camera.h` / `.cpp` — following the ball and converting screen/world positions.
- `GroundManager.h` / `.cpp` — creating and removing physics floor chunks.
- `BallSprites.h` — sprite rectangles for the ball designs.

## References

- [SDL3 CMake integration](https://wiki.libsdl.org/SDL3/README-cmake)
- [SDL3 entry points](https://wiki.libsdl.org/SDL3/README-main-functions)
- [Box2D v3.1.1 source and documentation](https://github.com/erincatto/box2d/tree/v3.1.1)
