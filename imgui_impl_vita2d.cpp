#include <vita2d.h>
#include <math.h>

#include "imgui_impl_vita2d.h"
#include "imgui_vita_touch.h"


extern SceGxmProgram _binary_assets_imgui_v_cg_gxp_start;
extern SceGxmProgram _binary_assets_imgui_f_cg_gxp_start;

static vita2d_texture *g_FontTexture = 0;
static uint64_t        g_Time = 0;
static bool            g_MousePressed[3] = { false, false, false };
static SceCtrlData     g_OldPad;
static int             hires_x = 0;
static int             hires_y = 0;
static bool touch_usage = false;
static bool mousestick_usage = true;
static bool gamepad_usage = false;
static uint32_t previous_down = 0;
static int repeat_count = 0;
static uint64_t previous_time = 0;
static uint32_t disabled_buttons = 0;

#define ANALOG_CENTER 128
#define ANALOG_THRESHOLD 64
#define BUTTON_LEFT 0x00000010
#define BUTTON_RIGHT 0x00000020
#define BUTTON_UP 0x00000040
#define BUTTON_DOWN 0x00000080

namespace
{
        void matrix_init_orthographic(
            float *m,
            float left,
            float right,
            float bottom,
            float top,
            float near,
            float far)
        {
                m[0x0] = 2.0f / (right - left);
                m[0x4] = 0.0f;
                m[0x8] = 0.0f;
                m[0xC] = -(right + left) / (right - left);

                m[0x1] = 0.0f;
                m[0x5] = 2.0f / (top - bottom);
                m[0x9] = 0.0f;
                m[0xD] = -(top + bottom) / (top - bottom);

                m[0x2] = 0.0f;
                m[0x6] = 0.0f;
                m[0xA] = -2.0f / (far - near);
                m[0xE] = (far + near) / (far - near);

                m[0x3] = 0.0f;
                m[0x7] = 0.0f;
                m[0xB] = 0.0f;
                m[0xF] = 1.0f;
        }

        SceGxmShaderPatcherId imguiVertexProgramId;
        SceGxmShaderPatcherId imguiFragmentProgramId;

        SceGxmVertexProgram *_vita2d_imguiVertexProgram;
        SceGxmFragmentProgram *_vita2d_imguiFragmentProgram;

        const SceGxmProgramParameter *_vita2d_imguiWvpParam;

        float ortho_proj_matrix[16];

        constexpr auto ImguiVertexSize = 20;
} // namespace

IMGUI_API void ImGui_ImplVita2D_Init()
{
        uint32_t err;

        ImGui_ImplVita2D_CreateDeviceObjects();

        // check the shaders
        err = sceGxmProgramCheck(&_binary_assets_imgui_v_cg_gxp_start);
        err = sceGxmProgramCheck(&_binary_assets_imgui_f_cg_gxp_start);

        // register programs with the patcher
        err = sceGxmShaderPatcherRegisterProgram(
            vita2d_get_shader_patcher(),
            &_binary_assets_imgui_v_cg_gxp_start,
            &imguiVertexProgramId);

        err = sceGxmShaderPatcherRegisterProgram(
            vita2d_get_shader_patcher(),
            &_binary_assets_imgui_f_cg_gxp_start,
            &imguiFragmentProgramId);

        // get attributes by name to create vertex format bindings
        const SceGxmProgramParameter *paramTexturePositionAttribute =
            sceGxmProgramFindParameterByName(
                &_binary_assets_imgui_v_cg_gxp_start, "aPosition");

        const SceGxmProgramParameter *paramTextureTexcoordAttribute =
            sceGxmProgramFindParameterByName(
                &_binary_assets_imgui_v_cg_gxp_start, "aTexcoord");

        const SceGxmProgramParameter *paramTextureColorAttribute =
            sceGxmProgramFindParameterByName(
                &_binary_assets_imgui_v_cg_gxp_start, "aColor");

        // create texture vertex format
        SceGxmVertexAttribute textureVertexAttributes[3];
        SceGxmVertexStream textureVertexStreams[1];
        /* x,y,z: 3 float 32 bits */
        textureVertexAttributes[0].streamIndex = 0;
        textureVertexAttributes[0].offset = 0;
        textureVertexAttributes[0].format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
        textureVertexAttributes[0].componentCount = 2; // (x, y)
        textureVertexAttributes[0].regIndex =
            sceGxmProgramParameterGetResourceIndex(
                paramTexturePositionAttribute);
        /* u,v: 2 floats 32 bits */
        textureVertexAttributes[1].streamIndex = 0;
        textureVertexAttributes[1].offset = 8; // (x, y) * 4 = 12 bytes
        textureVertexAttributes[1].format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
        textureVertexAttributes[1].componentCount = 2; // (u, v)
        textureVertexAttributes[1].regIndex =
            sceGxmProgramParameterGetResourceIndex(
                paramTextureTexcoordAttribute);
        /* r,g,b,a: 4 int 8 bits */
        textureVertexAttributes[2].streamIndex = 0;
        textureVertexAttributes[2].offset =
            16; // (x, y) * 4 + (u, v) * 4 = 20 bytes
        textureVertexAttributes[2].format = SCE_GXM_ATTRIBUTE_FORMAT_U8N;
        textureVertexAttributes[2].componentCount = 4; // (r, g, b, a)
        textureVertexAttributes[2].regIndex =
            sceGxmProgramParameterGetResourceIndex(paramTextureColorAttribute);
        // 16 bit (short) indices
        textureVertexStreams[0].stride = ImguiVertexSize;
        textureVertexStreams[0].indexSource = SCE_GXM_INDEX_SOURCE_INDEX_16BIT;

        // create texture shaders
        err = sceGxmShaderPatcherCreateVertexProgram(
            vita2d_get_shader_patcher(),
            imguiVertexProgramId,
            textureVertexAttributes,
            3,
            textureVertexStreams,
            1,
            &_vita2d_imguiVertexProgram);

        // Fill SceGxmBlendInfo
        static SceGxmBlendInfo blend_info{};
        blend_info.colorFunc = SCE_GXM_BLEND_FUNC_ADD;
        blend_info.alphaFunc = SCE_GXM_BLEND_FUNC_ADD;
        blend_info.colorSrc = SCE_GXM_BLEND_FACTOR_SRC_ALPHA;
        blend_info.colorDst = SCE_GXM_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        blend_info.alphaSrc = SCE_GXM_BLEND_FACTOR_SRC_ALPHA;
        blend_info.alphaDst = SCE_GXM_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        blend_info.colorMask = SCE_GXM_COLOR_MASK_ALL;

        err = sceGxmShaderPatcherCreateFragmentProgram(
            vita2d_get_shader_patcher(),
            imguiFragmentProgramId,
            SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4,
            SCE_GXM_MULTISAMPLE_NONE,
            &blend_info,
            &_binary_assets_imgui_v_cg_gxp_start,
            &_vita2d_imguiFragmentProgram);

        // find vertex uniforms by name and cache parameter information
        _vita2d_imguiWvpParam = sceGxmProgramFindParameterByName(
            &_binary_assets_imgui_v_cg_gxp_start, "wvp");

        matrix_init_orthographic(
            ortho_proj_matrix, 0.0f, VITA_WIDTH, VITA_HEIGHT, 0.0f, 0.0f, 1.0f);

        sceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT, SCE_TOUCH_SAMPLING_STATE_START);
        sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG_WIDE);

        // Setup back-end capabilities flags
        ImGuiIO &io = ImGui::GetIO();
        io.MouseDrawCursor = true;
        io.ClipboardUserData = NULL;

        ImGui_ImplVita2D_InitTouch();
}

IMGUI_API void ImGui_ImplVita2D_RenderDrawData(ImDrawData *draw_data)
{
        auto _vita2d_context = vita2d_get_context();

        sceGxmSetVertexProgram(_vita2d_context, _vita2d_imguiVertexProgram);
        sceGxmSetFragmentProgram(_vita2d_context, _vita2d_imguiFragmentProgram);

        void *vertex_wvp_buffer;
        sceGxmReserveVertexDefaultUniformBuffer(
            vita2d_get_context(), &vertex_wvp_buffer);
        sceGxmSetUniformDataF(
            vertex_wvp_buffer, _vita2d_imguiWvpParam, 0, 16, ortho_proj_matrix);

        for (int n = 0; n < draw_data->CmdListsCount; n++)
        {
                const auto cmd_list = draw_data->CmdLists[n];
                const auto vtx_buffer = cmd_list->VtxBuffer.Data;
                const auto vtx_size = cmd_list->VtxBuffer.Size;
                const auto idx_buffer = cmd_list->IdxBuffer.Data;
                const auto idx_size = cmd_list->IdxBuffer.Size;

                const auto vertices = vita2d_pool_memalign(
                    vtx_size * ImguiVertexSize, ImguiVertexSize);
                memcpy(vertices, vtx_buffer, vtx_size * ImguiVertexSize);

                static_assert(sizeof(ImDrawIdx) == 2);
                auto indices = (uint16_t *)vita2d_pool_memalign(
                    idx_size * sizeof(ImDrawIdx), sizeof(void *));
                memcpy(indices, idx_buffer, idx_size * sizeof(ImDrawIdx));

                auto err = sceGxmSetVertexStream(_vita2d_context, 0, vertices);

                for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
                {
                        const auto pcmd = &cmd_list->CmdBuffer[cmd_i];
                        if (pcmd->UserCallback)
                        {
                                pcmd->UserCallback(cmd_list, pcmd);
                        }
                        else
                        {
                                const auto texture = (vita2d_texture *)pcmd->TextureId;
                                err = sceGxmSetFragmentTexture(
                                    _vita2d_context, 0, &texture->gxm_tex);

                                err = sceGxmDraw(
                                    _vita2d_context,
                                    SCE_GXM_PRIMITIVE_TRIANGLES,
                                    SCE_GXM_INDEX_FORMAT_U16,
                                    indices,
                                    pcmd->ElemCount);
                        }
                        indices += pcmd->ElemCount;
                }
        }
}

IMGUI_API bool ImGui_ImplVita2D_CreateDeviceObjects()
{
        // Build texture atlas
        ImGuiIO &io = ImGui::GetIO();
        // Build and load the texture atlas into a texture
        uint32_t* pixels = NULL;
        int width, height;

        ImFontConfig font_config;
        font_config.OversampleH = 1;
        font_config.OversampleV = 1;
        font_config.PixelSnapH = 1;

        io.Fonts->AddFontFromFileTTF(
                    "sa0:/data/font/pvf/jpn0.pvf",
                    16.0f,
                    &font_config,
                    io.Fonts->GetGlyphRangesJapanese());
        io.Fonts->AddFontFromFileTTF(
                    "sa0:/data/font/pvf/ltn0.pvf",
                    16.0f,
                    &font_config,
                    io.Fonts->GetGlyphRangesDefault());
        io.Fonts->GetTexDataAsRGBA32((uint8_t**)&pixels, &width, &height);
        g_FontTexture =
                vita2d_create_empty_texture(width, height);
        const auto stride = vita2d_texture_get_stride(g_FontTexture) / 4;
        auto texture_data = (uint32_t*)vita2d_texture_get_datap(g_FontTexture);

        for (auto y = 0; y < height; ++y)
            for (auto x = 0; x < width; ++x)
                texture_data[y * stride + x] = pixels[y * width + x];

        io.Fonts->TexID = g_FontTexture;

        return true;
}

IMGUI_API void ImGui_ImplVita2D_InvalidateDeviceObjects()
{
        if (g_FontTexture)
        {
                vita2d_free_texture(g_FontTexture);
                ImGui::GetIO().Fonts->TexID = 0;
        }
}

IMGUI_API void ImGui_ImplVita2D_Shutdown()
{
        // Destroy Open2D objects
        ImGui_ImplVita2D_InvalidateDeviceObjects();
}

int mx, my;

void IN_RescaleAnalog(int *x, int *y, int dead)
{

        float analogX = (float)*x;
        float analogY = (float)*y;
        float deadZone = (float)dead;
        float maximum = 32768.0f;
        float magnitude = sqrt(analogX * analogX + analogY * analogY);
        if (magnitude >= deadZone)
        {
                float scalingFactor = maximum / magnitude * (magnitude - deadZone) / (maximum - deadZone);
                *x = (int)(analogX * scalingFactor);
                *y = (int)(analogY * scalingFactor);
        }
        else
        {
                *x = 0;
                *y = 0;
        }
}

void ImGui_ImplVita2D_PollLeftStick(SceCtrlData *pad, int *x, int *y)
{
        sceCtrlPeekBufferPositive(0, pad, 1);
        int lx = (pad->lx - 127) * 256;
        int ly = (pad->ly - 127) * 256;
        IN_RescaleAnalog(&lx, &ly, 7680);
        hires_x += lx;
        hires_y += ly;
        if (hires_x != 0 || hires_y != 0)
        {
                // slow down pointer, could be made user-adjustable
                int slowdown = 2048;
                *x += hires_x / slowdown;
                *y += hires_y / slowdown;
                hires_x %= slowdown;
                hires_y %= slowdown;
        }
}

IMGUI_API void ImGui_ImplVita2D_NewFrame()
{

        ImGuiIO &io = ImGui::GetIO();

        // Setup display size (every frame to accommodate for window resizing)
        io.DisplaySize = ImVec2((float)VITA_WIDTH, (float)VITA_HEIGHT);

        static uint64_t frequency = 1000000;
        uint64_t current_time = sceKernelGetProcessTimeWide();
        io.DeltaTime = g_Time > 0 ? (float)((double)(current_time - g_Time) / frequency) : (float)(1.0f / 60.0f);
        g_Time = current_time;

        // Touch for mouse emulation
        if (touch_usage)
        {
                double scale_x = 960.0 / io.DisplaySize.x;
                double scale_y = 544.0 / io.DisplaySize.y;
                double offset_x = 0;
                double offset_y = 0;
                ImGui_ImplVita2D_PollTouch(offset_x, offset_y, scale_x, scale_y, &mx, &my, g_MousePressed);
        }

        // Keypad navigation
        if (gamepad_usage)
        {
                SceCtrlData pad;
                int lstick_x, lstick_y = 0;
                ImGui_ImplVita2D_PollLeftStick(&pad, &lstick_x, &lstick_y);
                if (!(disabled_buttons & SCE_CTRL_CROSS))
                        io.NavInputs[ImGuiNavInput_Activate] = (pad.buttons & SCE_CTRL_CROSS) ? 1.0f : 0.0f;
                if (!(disabled_buttons & SCE_CTRL_CIRCLE))
                        io.NavInputs[ImGuiNavInput_Cancel] = (pad.buttons & SCE_CTRL_CIRCLE) ? 1.0f : 0.0f;
                if (!(disabled_buttons & SCE_CTRL_TRIANGLE))
                        io.NavInputs[ImGuiNavInput_Input] = (pad.buttons & SCE_CTRL_TRIANGLE) ? 1.0f : 0.0f;
                if (!(disabled_buttons & SCE_CTRL_SQUARE))
                        io.NavInputs[ImGuiNavInput_Menu] = (pad.buttons & SCE_CTRL_SQUARE) ? 1.0f : 0.0f;
                if (!(disabled_buttons & SCE_CTRL_LEFT))
                        io.NavInputs[ImGuiNavInput_DpadLeft] = (pad.buttons & SCE_CTRL_LEFT) ? 1.0f : 0.0f;
                if (!(disabled_buttons & SCE_CTRL_RIGHT))
                        io.NavInputs[ImGuiNavInput_DpadRight] = (pad.buttons & SCE_CTRL_RIGHT) ? 1.0f : 0.0f;
                if (!(disabled_buttons & SCE_CTRL_UP))
                        io.NavInputs[ImGuiNavInput_DpadUp] = (pad.buttons & SCE_CTRL_UP) ? 1.0f : 0.0f;
                if (!(disabled_buttons & SCE_CTRL_DOWN))
                        io.NavInputs[ImGuiNavInput_DpadDown] = (pad.buttons & SCE_CTRL_DOWN) ? 1.0f : 0.0f;

                if (io.NavInputs[ImGuiNavInput_Menu] == 1.0f)
                {
                        io.NavInputs[ImGuiNavInput_FocusPrev] = (pad.buttons & SCE_CTRL_LTRIGGER) ? 1.0f : 0.0f;
                        io.NavInputs[ImGuiNavInput_FocusNext] = (pad.buttons & SCE_CTRL_RTRIGGER) ? 1.0f : 0.0f;
                        if (lstick_x < 0)
                                io.NavInputs[ImGuiNavInput_LStickLeft] = (float)(-lstick_x / 16);
                        if (lstick_x > 0)
                                io.NavInputs[ImGuiNavInput_LStickRight] = (float)(lstick_x / 16);
                        if (lstick_y < 0)
                                io.NavInputs[ImGuiNavInput_LStickUp] = (float)(-lstick_y / 16);
                        if (lstick_y > 0)
                                io.NavInputs[ImGuiNavInput_LStickDown] = (float)(lstick_y / 16);
                }
                else if (!mousestick_usage)
                {
                        uint32_t down = 0;
                        if (pad.lx < ANALOG_CENTER - ANALOG_THRESHOLD)
                                down |= BUTTON_LEFT;
                        if (pad.lx > ANALOG_CENTER + ANALOG_THRESHOLD)
                                down |= BUTTON_RIGHT;
                        if (pad.ly < ANALOG_CENTER - ANALOG_THRESHOLD)
                                down |= BUTTON_UP;
                        if (pad.ly > ANALOG_CENTER + ANALOG_THRESHOLD)
                                down |= BUTTON_DOWN;

                        uint32_t pressed = down & ~previous_down;
                        if (previous_down == down)
                        {
                                uint64_t delay = 300000;
                                if (repeat_count > 0)
                                        delay = 100000;
                                if (current_time - previous_time > delay)
                                {
                                        pressed = down;
                                        previous_time = current_time;
                                        repeat_count++;
                                }
                        }
                        else
                        {
                                previous_time = current_time;
                                repeat_count = 0;
                        }
                        
                        if (pressed & BUTTON_LEFT)
                                io.NavInputs[ImGuiNavInput_DpadLeft] = 1.0f;
                        if (pressed & BUTTON_RIGHT)
                                io.NavInputs[ImGuiNavInput_DpadRight] = 1.0f;
                        if (pressed & BUTTON_UP)
                                io.NavInputs[ImGuiNavInput_DpadUp] = 1.0f;
                        if (pressed & BUTTON_DOWN)
                                io.NavInputs[ImGuiNavInput_DpadDown] = 1.0f;
                        previous_down = down;
                }
                
        }

        // Keys for mouse emulation
        if (mousestick_usage && !(io.NavInputs[ImGuiNavInput_Menu] == 1.0f))
        {
                SceCtrlData pad;
                ImGui_ImplVita2D_PollLeftStick(&pad, &mx, &my);
                if ((pad.buttons & SCE_CTRL_LTRIGGER) != (g_OldPad.buttons & SCE_CTRL_LTRIGGER))
                        g_MousePressed[0] = pad.buttons & SCE_CTRL_LTRIGGER;
                if ((pad.buttons & SCE_CTRL_RTRIGGER) != (g_OldPad.buttons & SCE_CTRL_RTRIGGER))
                        g_MousePressed[1] = pad.buttons & SCE_CTRL_RTRIGGER;
                g_OldPad = pad;
        }

        // Setup mouse inputs (we already got mouse wheel, keyboard keys & characters from our event handler)
        //Uint32 mouse_buttons = SDL_GetMouseState(&mx, &my);
        io.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);
        io.MouseDown[0] = g_MousePressed[0];
        io.MouseDown[1] = g_MousePressed[1];
        io.MouseDown[2] = g_MousePressed[2];

        if (mx < 0)
                mx = 0;
        else if (mx > 960)
                mx = 960;
        if (my < 0)
                my = 0;
        else if (my > 544)
                my = 544;
        io.MousePos = ImVec2((float)mx, (float)my);

        // Start the frame. This call will update the io.WantCaptureMouse, io.WantCaptureKeyboard flag that you can use to dispatch inputs (or not) to your application.
        ImGui::NewFrame();
}

void ImGui_ImplVita2D_TouchUsage(bool val)
{
        touch_usage = val;
}

void ImGui_ImplVita2D_UseIndirectFrontTouch(bool val)
{
        ImGui_ImplVita2D_PrivateSetIndirectFrontTouch(val);
}

void ImGui_ImplVita2D_UseRearTouch(bool val)
{
        ImGui_ImplVita2D_PrivateSetRearTouch(val);
}

void ImGui_ImplVita2D_MouseStickUsage(bool val)
{
        mousestick_usage = val;
}

void ImGui_ImplVita2D_GamepadUsage(bool val)
{
        gamepad_usage = val;
}

void ImGui_ImplVita2D_DisableButtons(uint32_t buttons)
{
        disabled_buttons = buttons;
}