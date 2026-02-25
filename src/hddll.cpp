#define WIN32_LEAN_AND_MEAN
#include "hddll/hddll.h"

#include <cstdint>
#include <thread>

#include "hddll/hooks.h"
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

void Start(HMODULE instance) {
  DisableThreadLibraryCalls(instance);
  const auto thread =
      CreateThread(nullptr, 0, MainThread, instance, 0, nullptr);
  if (thread) {
    CloseHandle(thread);
  }
}

} // namespace hddll
