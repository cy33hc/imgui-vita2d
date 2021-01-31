#include <vitasdk.h>
#include "imgui.h"

#define VITA_WIDTH 960
#define VITA_HEIGHT 544

IMGUI_API void        ImGui_ImplVita2D_Init();
IMGUI_API void        ImGui_ImplVita2D_Shutdown();
IMGUI_API void        ImGui_ImplVita2D_NewFrame();
IMGUI_API void        ImGui_ImplVita2D_RenderDrawData(ImDrawData* draw_data);

// Use if you want to reset your rendering device without losing ImGui state.
IMGUI_API void        ImGui_ImplVita2D_InvalidateDeviceObjects();
IMGUI_API bool        ImGui_ImplVita2D_CreateDeviceObjects();

void ImGui_ImplVita2D_TouchUsage(bool val);

// indirect front touch enabled: drag pointer with finger
// indirect front touch disabled: pointer jumps to finger
void ImGui_ImplVita2D_UseIndirectFrontTouch(bool val);

 // turn rear panel touch on or off
void ImGui_ImplVita2D_UseRearTouch(bool val);

// Left mouse stick and trigger buttons control  mouse pointer
void ImGui_ImplVita2D_MouseStickUsage(bool val);

// GamepadUsage uses the Vita buttons to navigate and interact with UI elements
void ImGui_ImplVita2D_GamepadUsage(bool val);

void ImGui_ImplVita2D_DisableButtons(unsigned int buttons);

void ImGui_ImplVita2D_SetAnalogRepeatDelay(int delay);

void ImGui_ImplVita2D_SwapXO(bool val);
