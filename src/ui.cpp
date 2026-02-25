#include "hddll/ui.h"

#include <imgui.h>
#include <imgui_impl_dx9.h>
#include <imgui_impl_win32.h>


#include <stdexcept>

#include "hddll/hddll.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND window,
                                                             UINT message,
                                                             WPARAM wideParam,
                                                             LPARAM longParam);

LRESULT CALLBACK WindowProcess(HWND window, UINT message, WPARAM wideParam,
                               LPARAM longParam);

namespace hddll {

bool ui::SetupWindowClass(const char *windowClassName) noexcept {
  windowClass.cbSize = sizeof(WNDCLASSEXA);
  windowClass.style = CS_HREDRAW | CS_VREDRAW;
  windowClass.lpfnWndProc = DefWindowProcA;
  windowClass.cbClsExtra = 0;
  windowClass.cbWndExtra = 0;
  windowClass.hInstance = GetModuleHandleA(NULL);
  windowClass.hIcon = NULL;
  windowClass.hCursor = NULL;
  windowClass.hbrBackground = NULL;
  windowClass.lpszMenuName = NULL;
  windowClass.lpszClassName = windowClassName;
  windowClass.hIconSm = NULL;

  if (!RegisterClassExA(&windowClass)) {
    return false;
  }

  return true;
}

void ui::DestroyWindowClass() noexcept {
  UnregisterClassA(windowClass.lpszClassName, windowClass.hInstance);
}

bool ui::SetupWindow(const char *windowName) noexcept {
  // Create temp window to get dx9 device
  window =
      CreateWindowA(windowClass.lpszClassName, windowName, WS_OVERLAPPEDWINDOW,
                    0, 0, 100, 100, 0, 0, windowClass.hInstance, 0);
  if (!window) {
    return false;
  }
  return true;
}

void ui::DestroyWindow() noexcept {
  if (window) {
    DestroyWindow(window);
  }
}

bool ui::SetupDirectX() noexcept {
  const auto handle = GetModuleHandleA("d3d9.dll");

  if (!handle) {
    return false;
  }

  using CreateFn = LPDIRECT3D9(__stdcall *)(UINT);
  const auto create =
      reinterpret_cast<CreateFn>(GetProcAddress(handle, "Direct3DCreate9"));

  if (!create) {
    return false;
  }

  d3d9 = create(D3D_SDK_VERSION);
  if (!d3d9) {
    return false;
  }

  D3DPRESENT_PARAMETERS params = {};
  params.BackBufferWidth = 0;
  params.BackBufferHeight = 0;
  params.BackBufferFormat = D3DFMT_UNKNOWN;
  params.BackBufferCount = 0;
  params.MultiSampleType = D3DMULTISAMPLE_NONE;
  params.MultiSampleQuality = NULL;
  params.SwapEffect = D3DSWAPEFFECT_DISCARD;
  params.hDeviceWindow = window;
  params.Windowed = 1;
  params.EnableAutoDepthStencil = 0;
  params.AutoDepthStencilFormat = D3DFMT_UNKNOWN;
  params.Flags = NULL;
  params.FullScreen_RefreshRateInHz = 0;
  params.PresentationInterval = 0;

  if (d3d9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_NULLREF, window,
                         D3DCREATE_SOFTWARE_VERTEXPROCESSING |
                             D3DCREATE_DISABLE_DRIVER_MANAGEMENT,
                         &params, &device) < 0) {
    return false;
  }

  return true;
}
void ui::DestroyDirectX() noexcept {
  if (device) {
    device->Release();
    device = NULL;
  }

  if (d3d9) {
    d3d9->Release();
    d3d9 = NULL;
  }
}

void ui::Setup() {

  if (!SetupWindowClass("HDDLLWindowClass")) {
    throw std::runtime_error("Failed to create window class.");
  }

  if (!SetupWindow("HDDLL Window")) {
    throw std::runtime_error("Failed to create window.");
  }

  if (!SetupDirectX()) {
    throw std::runtime_error("Failed to create device.");
  }

  DestroyWindow();
  DestroyWindowClass();
}

void ui::SetupMenu(LPDIRECT3DDEVICE9 device) noexcept {
  auto params = D3DDEVICE_CREATION_PARAMETERS{};
  device->GetCreationParameters(&params);

  window = params.hFocusWindow;
  originalWindowProcess = reinterpret_cast<WNDPROC>(SetWindowLongPtr(
      window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WindowProcess)));

  ImGui::CreateContext();
  ImGui::StyleColorsDark();

  ImGui_ImplWin32_Init(window);
  ImGui_ImplDX9_Init(device);

  hddll::init();
  onInit();

  setup = true;
}

void ui::Destroy() noexcept {
  ImGui_ImplDX9_Shutdown();
  ImGui_ImplWin32_Shutdown();
  ImGui::DestroyContext();

  SetWindowLongPtr(window, GWLP_WNDPROC,
                   reinterpret_cast<LONG_PTR>(originalWindowProcess));

  onDestroy();

  DestroyDirectX();
}

void ui::Render() noexcept {

  ImGui_ImplDX9_NewFrame();
  ImGui_ImplWin32_NewFrame();
  ImGui::NewFrame();

  hddll::updateState();
  onFrame();

  ImGui::EndFrame();
  ImGui::Render();
  ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
}

} // namespace hddll

LRESULT CALLBACK WindowProcess(HWND window, UINT message, WPARAM wideParam,
                               LPARAM longParam) {

  if (ImGui_ImplWin32_WndProcHandler(window, message, wideParam, longParam)) {
    return 1L;
  }

  return CallWindowProc(hddll::ui::originalWindowProcess, window, message,
                        wideParam, longParam);
}
