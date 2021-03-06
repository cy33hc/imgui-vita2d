// ImGui - standalone example application for SDL2 + OpenGL
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.
// (SDL is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan graphics context creation, etc.)

// **DO NOT USE THIS CODE IF YOUR CODE/ENGINE IS USING MODERN OPENGL (SHADERS, VBO, VAO, etc.)**
// **Prefer using the code in the sdl_opengl3_example/ folder**
// See imgui_impl_sdl.cpp for details.

#include <imgui_vita2d/imgui_vita.h>
#include <stdio.h>
#include <vita2d.h>

int main(int, char**)
{
	
	vita2d_init();

	// Setup ImGui binding
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui_ImplVita2D_Init();

	// Setup style
	ImGui::StyleColorsDark();

	bool show_demo_window = true;
	bool show_another_window = false;
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	ImGui_ImplVita2D_TouchUsage(true);
	ImGui_ImplVita2D_UseIndirectFrontTouch(false);
	ImGui_ImplVita2D_UseRearTouch(true);
	ImGui_ImplVita2D_GamepadUsage(true);

	// Main loop
	bool done = false;
	while (!done)
	{
		vita2d_start_drawing();
		vita2d_clear_screen();

		ImGui_ImplVita2D_NewFrame();
		
		if (ImGui::BeginMainMenuBar()){
			if (ImGui::BeginMenu("Debug")){
				ImGui::EndMenu();
			}
			ImGui::EndMainMenuBar();
		}
		
		// 1. Show a simple window.
		// Tip: if we don't call ImGui::Begin()/ImGui::End() the widgets automatically appears in a window called "Debug".
		{
			static float f = 0.0f;
			static int counter = 0;
			ImGui::Text("Hello, world!");						   // Display some text (you can use a format string too)
			ImGui::SliderFloat("float", &f, 0.0f, 1.0f);			// Edit 1 float using a slider from 0.0f to 1.0f	
			ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

			ImGui::Checkbox("Demo Window", &show_demo_window);	  // Edit bools storing our windows open/close state
			ImGui::Checkbox("Another Window", &show_another_window);

			if (ImGui::Button("Button"))							// Buttons return true when clicked (NB: most widgets return true when edited/activated)
				counter++;
			ImGui::SameLine();
			ImGui::Text("counter = %d", counter);

			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		}

		// 2. Show another simple window. In most cases you will use an explicit Begin/End pair to name your windows.
		if (show_another_window)
		{
			ImGui::Begin("Another Window", &show_another_window);
			ImGui::Text("Hello from another window!");
			if (ImGui::Button("Close Me"))
				show_another_window = false;
			ImGui::End();
		}

		// 3. Show the ImGui demo window. Most of the sample code is in ImGui::ShowDemoWindow(). Read its code to learn more about Dear ImGui!
		if (show_demo_window)
		{
			ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiCond_FirstUseEver); // Normally user code doesn't need/want to call this because positions are saved in .ini file anyway. Here we just want to make the demo initial state a bit more friendly!
			ImGui::ShowDemoWindow(&show_demo_window);
		}

		ImGui::Render();
		ImGui_ImplVita2D_RenderDrawData(ImGui::GetDrawData());

		vita2d_end_drawing();
		vita2d_swap_buffers();
		sceDisplayWaitVblankStart();
	}

	// Cleanup
	ImGui_ImplVita2D_Shutdown();
	ImGui::DestroyContext();
	
	vita2d_fini();

	return 0;
}
