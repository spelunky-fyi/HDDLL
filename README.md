# HDDLL

A reusable C++ static library for building DLLs to inject into Spelunky HD. Provides DLL injection bootstrapping, DirectX 9 hooking, ImGui overlay setup, memory patching utilities, and game memory layout.

## What's Included

- **DLL entry point** (`dllmain.cpp`) — thread setup and teardown
- **DirectX 9 hooks** (`hooks.cpp`) — EndScene/Reset hooking via MinHook
- **ImGui overlay** (`ui.cpp`) — window class registration, device creation, ImGui init/render loop
- **Memory patching** (`memory.cpp`) — code patching, byte hooks, force patches
- **Game memory layout** (`hd.h`, `hd_entity.h`) — `GlobalState`, `CameraState`, entity class hierarchy, and game function wrappers
- **Entity definitions** (`entities.h`) — entity type ID table

## Requirements

- CMake 3.13+
- MSVC with C++20 support
- Win32 (x86) target — Spelunky HD is a 32-bit application

## Setup

Clone with submodules:

```
git clone --recurse-submodules https://github.com/spelunky-fyi/HDDLL.git
```

Or if already cloned:

```
git submodule update --init --recursive
```

## Usage

### 1. Add HDDLL to your project

Add HDDLL as a subdirectory (or git submodule) in your mod project:

```cmake
add_subdirectory(path/to/HDDLL)

add_library(my_mod SHARED my_mod.cpp)
target_link_libraries(my_mod PRIVATE hddll)
```

Build with the Win32 architecture:

```
cmake -B build -A Win32
cmake --build build
```

### 2. Implement the required callbacks

Your mod must define three callback functions and the global variable definitions that HDDLL expects. These are declared as `extern` in `hddll.h`:

```cpp
#include "hddll.h"
#include "hd.h"

// Global variable definitions
DWORD gBaseAddress = 0;
GlobalState *gGlobalState = nullptr;
CameraState *gCameraState = nullptr;

// Called once after ImGui is initialized
void hddllOnInit() {
    // Set up gBaseAddress, gGlobalState, gCameraState, etc.
}

// Called every frame inside the ImGui render loop
void hddllOnFrame() {
    // Draw your ImGui UI here
}

// Called on teardown
void hddllOnDestroy() {
    // Clean up
}
```

### 3. Inject the DLL

The build produces a `.dll` from your shared library target. Inject it into a running Spelunky HD process using any standard DLL injector.

## DEV mode

Pass `-DDEV=ON` to CMake to enable development features. In DEV mode, the main thread stays alive and polls for `VK_END` to trigger a clean unload, which is useful for iterating without restarting the game:

```
cmake -B build -A Win32 -DDEV=ON
```

## API Overview

### hddll.h

The integration header. Include this in your mod to access the extern globals and see the callback signatures.

| Symbol             | Description                                   |
| ------------------ | --------------------------------------------- |
| `gBaseAddress`     | Base address of the Spelunky HD module        |
| `gGlobalState`     | Pointer to the game's `GlobalState`           |
| `gCameraState`     | Pointer to the game's `CameraState`           |
| `hddllOnInit()`    | Your callback — called once after ImGui setup |
| `hddllOnFrame()`   | Your callback — called each frame             |
| `hddllOnDestroy()` | Your callback — called on teardown            |

### memory.h

| Function                 | Description                                              |
| ------------------------ | -------------------------------------------------------- |
| `applyPatches()`         | Apply/rollback byte patches at offsets from base address |
| `applyRelativePatches()` | Apply/rollback relative address patches                  |
| `applyForcePatch()`      | Apply always/never/normal force patches                  |
| `hook()` / `unhook()`    | Install/remove inline JMP hooks                          |
| `cleanUpHooks()`         | Remove all installed inline hooks                        |

### hd.h

Game memory structures (`GlobalState`, `CameraState`, `LevelState`, `EntityStruct`, `PlayerData`, etc.) and game function wrappers (`SpawnEntity`, `SpawnHiredHand`, `DestroyFloor`, `setupOffsets`, etc.).

### hd_entity.h

Entity class hierarchy: `Entity`, `EntityActive`, `EntityPlayer`, `EntityMonster`, `EntityItem`, `EntityFloor`, `EntityBackground`, `EntityExplosion`.
