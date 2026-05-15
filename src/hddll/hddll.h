#pragma once

#include <Windows.h>

namespace hddll {

class GlobalState;
class CameraState;

extern DWORD gBaseAddress;
extern GlobalState *gGlobalState;
extern CameraState *gCameraState;
extern int gWindowedMode;
extern int gDisplayWidth;
extern int gDisplayHeight;

// World->NDC orthographic projection matrix (D3DXMATRIX, row-major float[16] at
// gBaseAddress + 0x15a5b8). The game builds it once at startup and re-uploads
// it to the shader every frame. Exposed as a neutral pointer like the state
// pointers above; SpecsHD's zoom feature is what actually mutates it.
extern float *gProjectionMatrix;

// World tiles visible across the viewport, relative to the game's native view.
// 1.0 = native (20 tiles wide); the screen<->game coordinate helpers honor this
// so debug overlays stay aligned when the view is zoomed. HDDLL only reads it --
// SpecsHD's zoom feature owns and drives it.
extern float gViewScale;

// Called automatically before onInit() and onFrame() respectively.
void init();
void updateState();

// Call from DllMain on DLL_PROCESS_ATTACH to start the HDDLL main thread.
void Start(HMODULE instance);

extern void onInit();
extern void onFrame();
extern void onDestroy();

} // namespace hddll
