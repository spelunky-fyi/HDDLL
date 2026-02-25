#define WIN32_LEAN_AND_MEAN
#include "hddll/hddll.h"

#include <cstdint>
#include <thread>

#include "hddll/hd.h"
#include "hddll/hooks.h"
#include "hddll/memory.h"
#include "hddll/ui.h"

static DWORD WINAPI MainThread(LPVOID instance) {
  srand(static_cast<unsigned int>(time(NULL)));

  try {
    hddll::ui::Setup();
    hddll::hooks::Setup();
  } catch (const std::exception &error) {
    MessageBeep(MB_ICONERROR);
    MessageBoxA(0, error.what(), "HDDLL Error", MB_OK | MB_ICONEXCLAMATION);
    goto DIE;
  }

#ifdef DEV
  while (!GetAsyncKeyState(VK_END)) {
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
  }
#else
  return 0;
#endif

DIE:
  hddll::hooks::Destroy();
  hddll::ui::Destroy();
  FreeLibraryAndExitThread(static_cast<HMODULE>(instance), 0);
  return 0;
}

namespace hddll {

DWORD gBaseAddress = NULL;
GlobalState *gGlobalState = NULL;
CameraState *gCameraState = NULL;
int gWindowedMode = 0;
int gDisplayWidth = 0;
int gDisplayHeight = 0;

void init() {
  gBaseAddress = (size_t)GetModuleHandleA(NULL);
  setupOffsets(gBaseAddress);

  auto process = GetCurrentProcess();

  BYTE patch[] = {0x4a};
  patchReadOnlyCode(process, gBaseAddress + 0x135B2A, patch, 1);

  BYTE patch2[] = {0xF0};
  patchReadOnlyCode(process, gBaseAddress + 0x1366C6, patch2, 1);
}

void updateState() {
  gCameraState =
      reinterpret_cast<CameraState *>(*((DWORD *)(gBaseAddress + 0x154510)));
  gGlobalState =
      reinterpret_cast<GlobalState *>(*((DWORD *)(gBaseAddress + 0x15446C)));

  gWindowedMode = static_cast<int>(*((DWORD *)(gBaseAddress + 0x15a52c)));
  gDisplayWidth = static_cast<int>(*((DWORD *)(gBaseAddress + 0x140a8c)));
  gDisplayHeight = static_cast<int>(*((DWORD *)(gBaseAddress + 0x140a90)));

  if (gGlobalState)
    gGlobalState->N00001004 = 0; // 440629
}

void Start(HMODULE instance) {
  DisableThreadLibraryCalls(instance);
  const auto thread =
      CreateThread(nullptr, 0, MainThread, instance, 0, nullptr);
  if (thread) {
    CloseHandle(thread);
  }
}

} // namespace hddll
