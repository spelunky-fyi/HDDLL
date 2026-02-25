# HDDLL

A reusable C++ static library for building DLLs to inject into Spelunky HD. Provides DLL injection bootstrapping, DirectX 9 hooking, ImGui overlay setup, memory patching utilities, and game memory layout.

## What's Included

- **DLL bootstrapping** (`hddll.h` / `hddll.cpp`) — `hddll::Start()` handles thread creation, DX9 hook setup, and teardown
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

Add HDDLL as a git submodule in your project:

```
git submodule add https://github.com/spelunky-fyi/HDDLL.git path/to/HDDLL
git submodule update --init --recursive
```

## Usage

### 1. Add HDDLL to your project

Add HDDLL as a subdirectory (or git submodule) in your mod project:

```cmake
add_subdirectory(path/to/HDDLL)

add_library(my_mod SHARED dllmain.cpp my_mod.cpp)
target_link_libraries(my_mod PRIVATE hddll)
```

Build with the Win32 architecture:

```
cmake -B build -A Win32
cmake --build build
```

### 2. Create your DLL entry point

HDDLL provides `hddll::Start()` which handles thread creation, DirectX 9 hooking, ImGui initialization, and teardown. Your `dllmain.cpp` just needs to call it:

```cpp
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <hddll/hddll.h>

BOOL WINAPI DllMain(HMODULE hMod, DWORD dwReason, LPVOID lpReserved) {
  if (dwReason == DLL_PROCESS_ATTACH) {
    hddll::Start(hMod);
  }
  return TRUE;
}
```

> **Note:** `DllMain` must be defined in your project, not in the static library, because MSVC's linker won't pull `DllMain` from a static library — the CRT provides a default no-op.

### 3. Implement the required callbacks

Your mod must define three callback functions and the global variable definitions that HDDLL expects. These are declared as `extern` inside `namespace hddll` in `hddll.h`:

```cpp
#include <hddll/hddll.h>
#include <hddll/hd.h>

// Global variable definitions (inside namespace hddll)
namespace hddll {
DWORD gBaseAddress = 0;
GlobalState *gGlobalState = nullptr;
CameraState *gCameraState = nullptr;

// Called once after ImGui is initialized
void onInit() {
    // Set up hddll::gBaseAddress, hddll::gGlobalState, hddll::gCameraState, etc.
}

// Called every frame inside the ImGui render loop
void onFrame() {
    // Draw your ImGui UI here
}

// Called on teardown
void onDestroy() {
    // Clean up
}
} // namespace hddll
```

### 4. Inject the DLL

The build produces a `.dll` from your shared library target. Inject it into a running Spelunky HD process using any standard DLL injector.

## DEV mode

Pass `-DDEV=ON` to CMake to enable development features. In DEV mode, the main thread stays alive and polls for `VK_END` to trigger a clean unload, which is useful for iterating without restarting the game:

```
cmake -B build -A Win32 -DDEV=ON
```

## API Overview

### hddll.h

The integration header. Include this in your mod to access the extern globals and see the callback signatures.

All symbols live inside `namespace hddll`.

| Symbol                  | Description                                          |
| ----------------------- | ---------------------------------------------------- |
| `hddll::Start()`        | Call from `DllMain` — sets up thread, hooks, and UI  |
| `hddll::gBaseAddress`   | Base address of the Spelunky HD module               |
| `hddll::gGlobalState`   | Pointer to the game's `GlobalState`                  |
| `hddll::gCameraState`   | Pointer to the game's `CameraState`                  |
| `hddll::onInit()`       | Your callback — called once after ImGui setup        |
| `hddll::onFrame()`      | Your callback — called each frame                    |
| `hddll::onDestroy()`    | Your callback — called on teardown                   |

### memory.h

| Function                          | Description                                              |
| --------------------------------- | -------------------------------------------------------- |
| `hddll::applyPatches()`          | Apply/rollback byte patches at offsets from base address |
| `hddll::applyRelativePatches()`  | Apply/rollback relative address patches                  |
| `hddll::applyForcePatch()`       | Apply always/never/normal force patches                  |
| `hddll::hook()` / `hddll::unhook()` | Install/remove inline JMP hooks                       |
| `hddll::cleanUpHooks()`          | Remove all installed inline hooks                        |

### hd.h

Game memory structures (`GlobalState`, `CameraState`, `LevelState`, `EntityStruct`, `PlayerData`, etc.) and game function wrappers (`SpawnEntity`, `SpawnHiredHand`, `DestroyFloor`, `setupOffsets`, etc.).

### hd_entity.h

Entity class hierarchy: `Entity`, `EntityActive`, `EntityPlayer`, `EntityMonster`, `EntityItem`, `EntityFloor`, `EntityBackground`, `EntityExplosion`.
