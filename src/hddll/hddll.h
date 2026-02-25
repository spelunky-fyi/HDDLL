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

// Called automatically before onInit() and onFrame() respectively.
void init();
void updateState();

// Call from DllMain on DLL_PROCESS_ATTACH to start the HDDLL main thread.
void Start(HMODULE instance);

extern void onInit();
extern void onFrame();
extern void onDestroy();

} // namespace hddll
