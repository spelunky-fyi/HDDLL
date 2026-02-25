#pragma once

#include <Windows.h>

class GlobalState;
class CameraState;

extern DWORD gBaseAddress;
extern GlobalState *gGlobalState;
extern CameraState *gCameraState;

extern void hddllOnInit();
extern void hddllOnFrame();
extern void hddllOnDestroy();
