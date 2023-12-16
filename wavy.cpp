#include <helpers.cpp>
#include <string>
#include <iostream>
#include <conio.h>
#include "common.h"
#include "bgfx_utils.h"
#include "imgui/imgui.h"
#include <bx/uint32_t.h>
#include <math.h>

namespace
{
    struct CircleVertex
    {
        float m_x;
        float m_y;
        float m_z;
        uint32_t m_abgr;

        static void init()
        {
            ms_layout
                .begin()
                .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
                .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
                .end();
        };

        static bgfx::VertexLayout ms_layout;
    };
    bgfx::VertexLayout CircleVertex::ms_layout;

    static const char *s_ptNames[]{
        "Triangle List",
        "Triangle Strip",
        "Lines",
        "Line Strip",
        "Points",
    };

    static const uint64_t s_ptState[]{
        UINT64_C(0),
        BGFX_STATE_PT_TRISTRIP,
        BGFX_STATE_PT_LINES,
        BGFX_STATE_PT_LINESTRIP,
        BGFX_STATE_PT_POINTS,
    };
    BX_STATIC_ASSERT(BX_COUNTOF(s_ptState) == BX_COUNTOF(s_ptNames));

    // Create a vertex buffer for the circle
    static void createCircleVB(bgfx::VertexBufferHandle *_result, float radius, uint32_t color, uint16_t segments)
    {
        // Calculate the number of vertices and the size of the buffer
        uint16_t numVertices = segments + 1;
        uint32_t size = numVertices * sizeof(CircleVertex);

        // Get a pointer to the vertex data
        CircleVertex *vertex = new CircleVertex[numVertices];

        // Add the center vertex
        vertex[0] = CircleVertex{
            0.0f,
            0.0f,
            0.0f,
            color};

        // Add the perimeter vertices
        float angleStep = 2.0f * M_PI / segments;
        float angle = 0.0f;
        for (uint16_t i = 1; i < segments + 1; i++)
        {
            vertex[i] = CircleVertex{
                radius * cosf(angle),
                radius * sinf(angle),
                0.0f,
                color};
            angle += angleStep;
        }
        debug("Vertex Buffer: \n");
        for (int i = 0; i < numVertices; i++)
        {
            debug("\t{ %f %f %f %u }\n", vertex[i].m_x, vertex[i].m_y, vertex[i].m_z, vertex[i].m_abgr);
        }
        debug("\n");

        // Return the vertex buffer handle
        *_result = bgfx::createVertexBuffer(bgfx::copy(vertex, size), CircleVertex::ms_layout);
    }

    // Create a index buffer for the circle
    static void createCircleIB(bgfx::IndexBufferHandle *_result, uint16_t segments)
    {
        // Calculate the number of indices and the size of the buffer
        uint16_t numIndices = segments * 3;
        uint32_t size = numIndices * sizeof(uint16_t);

        // Get a pointer to the index data
        uint16_t *index = new uint16_t[numIndices];
        int i_offset = 0;
        // Add the triangle indices
        for (uint16_t i = 0; i < segments; i++)
        {
            index[i_offset] = 0;         // Center vertex
            index[i_offset + 1] = i + 1; // Current perimeter vertex
            index[i_offset + 2] = i + 2; // Next perimeter vertex
            i_offset += 3;
        }
        // Fix the last triangle
        index[numIndices - 1] = 1;
        debug("Index Buffer:\n\t{ ");
        for (int i = 0; i < numIndices; i++)
        {
            debug("%d ", index[i]);
        }
        debug("}\n");
        // Return the transient index buffer
        *_result = bgfx::createIndexBuffer(bgfx::copy(index, size));
    }

    class Wavy : public entry::AppI
    {
    public:
        Wavy(const char *_name, const char *_description, const char *_url)
            : entry::AppI(_name, _description, _url), m_pt(4), m_r(true), m_g(true), m_b(true), m_a(true)
        {
        }

        void init(int32_t _argc, const char *const *_argv, uint32_t _width, uint32_t _height) override
        {
            Args args(_argc, _argv);

            WIDTH = _width;
            HEIGHT = _height;
            m_debug = BGFX_DEBUG_TEXT;
            m_reset = BGFX_RESET_VSYNC;
            std::cout << "width: " << _width << " height: " << _height << std::endl;
            
            y_ar = m_eye_z * float(HEIGHT) / float(WIDTH);

            bgfx::Init init;
            init.type = args.m_type;
            init.vendorId = args.m_pciId;
            init.platformData.nwh = entry::getNativeWindowHandle(entry::kDefaultWindowHandle);
            init.platformData.ndt = entry::getNativeDisplayHandle();
            init.platformData.type = entry::getNativeWindowHandleType();
            init.resolution.width = WIDTH;
            init.resolution.height = HEIGHT;
            init.resolution.reset = m_reset;

            if (!bgfx::init(init))
            {
                return;
            }

            // Enable debug text.
            bgfx::setDebug(m_debug);

            // Create vertex stream declaration.

            // Set view 0 clear state.
            bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x303030ff, 1.0f, 0);

            // Create vertex stream declaration.
            CircleVertex::init();

            // Create a transient vertex buffer and index buffer for the circle
            createCircleVB(&m_vbh, m_radius, dot_color, dot_resolution);
            createCircleIB(&m_ibh, dot_resolution);

            // Create program from shaders.
            m_program = loadProgram("vs_wavy", "fs_wavy");
            //m_program = loadProgram("vs_cubes", "fs_cubes");

            m_timeOffset = bx::getHPCounter();

            imguiCreate();
            
        }

        virtual int shutdown() override
        {
            imguiDestroy();
            bgfx::destroy(m_ibh);

            bgfx::destroy(m_vbh);
            bgfx::destroy(m_program);

            // Shutdown bgfx.
            bgfx::shutdown();

            return 0;
        }

        bool update() override
        {  

            if (!entry::processEvents(WIDTH, HEIGHT, m_debug, m_reset, &m_mouseState))
            {
                imguiBeginFrame(m_mouseState.m_mx, m_mouseState.m_my, (m_mouseState.m_buttons[entry::MouseButton::Left] ? IMGUI_MBUT_LEFT : 0) | (m_mouseState.m_buttons[entry::MouseButton::Right] ? IMGUI_MBUT_RIGHT : 0) | (m_mouseState.m_buttons[entry::MouseButton::Middle] ? IMGUI_MBUT_MIDDLE : 0), m_mouseState.m_mz, uint16_t(WIDTH), uint16_t(HEIGHT));

                ImVec2 window_size = ImGui::GetWindowSize();
                if (window_size.x != WIDTH || window_size.y != HEIGHT) update_view();

                showExampleDialog(this);

                ImGui::SetNextWindowPos(
                    ImVec2(WIDTH - WIDTH / 5.0f - 10.0f, 10.0f), ImGuiCond_FirstUseEver);
                ImGui::SetNextWindowSize(
                    ImVec2(WIDTH / 5.0f, HEIGHT / 3.5f), ImGuiCond_FirstUseEver);
                ImGui::Begin("Settings", NULL, 0);

                ImGui::Checkbox("Write R", &m_r);
                ImGui::Checkbox("Write G", &m_g);
                ImGui::Checkbox("Write B", &m_b);
                ImGui::Checkbox("Write A", &m_a);

                ImGui::Text("Primitive topology:");
                ImGui::Combo("##topology", (int *)&m_pt, s_ptNames, BX_COUNTOF(s_ptNames));

                ImGui::End();
                imguiEndFrame();

                float time = (float)((bx::getHPCounter() - m_timeOffset) / double(bx::getHPFrequency()));

                // This dummy draw call is here to make sure that view 0 is cleared
                // if no other draw calls are submitted to view 0.
                bgfx::touch(0);
                float m_x = 0.0f;
                float m_y = 0.0f;
                // Use debug to print information about this example.
                if (m_mouseState.m_buttons[entry::MouseButton::Left])
                {
                    m_x = (bound * m_eye_z * (2 * float(m_mouseState.m_mx) - WIDTH)) / WIDTH;
                    m_y = (bound * y_ar * (HEIGHT - 2*float(m_mouseState.m_my))) / HEIGHT;
                    debug("\r%d %d ->  %f %f", m_mouseState.m_mx, m_mouseState.m_my, m_x, m_y);
                }

                drawCircle();
                // Advance to next frame. Rendering thread will be kicked to
                // process submitted rendering primitives.
                bgfx::frame();

                return true;
            }
            return false;
        }

        // Draw a filled circle with the given parameters
        void drawCircle()
        {
            uint64_t state = 0 | (m_r ? BGFX_STATE_WRITE_R : 0) | (m_g ? BGFX_STATE_WRITE_G : 0) | (m_b ? BGFX_STATE_WRITE_B : 0) | (m_a ? BGFX_STATE_WRITE_A : 0) | BGFX_STATE_WRITE_Z | BGFX_STATE_DEPTH_TEST_LESS | BGFX_STATE_CULL_CW | BGFX_STATE_MSAA | s_ptState[m_pt];
            
            int x_count = grid_size;
            int y_count = grid_size * HEIGHT / WIDTH;

            for (int y = 0; y < y_count; y++){
                for (int x = 0; x < x_count; x++){
                    // convert x,y from world space to screen space coordinates
                    float xx = (bound * m_eye_z * ( 2*( float(x) * (float(WIDTH) / float(x_count-1))) - WIDTH)) / WIDTH;
                    float yy = (bound * y_ar * (HEIGHT - 2*(float(y) * (float(HEIGHT)/float(y_count-1))))) / HEIGHT;

                    // evenly disperse the circles across the screen

                    float mtx[16];
                    bx::mtxTranslate(mtx, xx, yy, 0.0f);

                    // Set model matrix for rendering.
                    bgfx::setTransform(mtx);

                    // Set vertex and index buffer.
                    bgfx::setVertexBuffer(0, m_vbh);
                    bgfx::setIndexBuffer(m_ibh);

                    // Set render states.
                    bgfx::setState(state);

                    // Submit primitive for rendering to view 0.
                    bgfx::submit(main_view_id, m_program);
                }
            }
        }
        void update_view()
        {
            const bx::Vec3 at = {0.0f, 0.0f, 0.0f};
            const bx::Vec3 eye = {0.0f, 0.0f, -m_eye_z};
            
            float proj[16];
            float view[16];
            float inv_proj[16];
            float world_pos[4] = {1.0f, 1.0f, 0.0f, 0.0f};
            // Set view and projection matrix for view 0.
            {
                bx::mtxLookAt(view, eye, at);

                bx::mtxProj(proj, fov, float(WIDTH) / float(HEIGHT), 0.1f, 100.0f, bgfx::getCaps()->homogeneousDepth);
                
                bgfx::setViewTransform(main_view_id, view, proj);
                
                // This is set to determine the size of the drawable surface
                bgfx::setViewRect(main_view_id, 0, 0, uint16_t(WIDTH), uint16_t(HEIGHT));
            }

            bx::mtxInverse(inv_proj, proj);
            bx::mtxMul(world_pos, inv_proj, world_pos);
            bound = world_pos[0];
        }

        entry::MouseState m_mouseState;

        uint32_t m_debug;
        uint32_t m_reset;
        bgfx::ProgramHandle m_program;
        bgfx::VertexBufferHandle m_vbh;
        bgfx::IndexBufferHandle m_ibh;

        uint32_t WIDTH;
        uint32_t HEIGHT;
        int64_t m_timeOffset;
        CircleVertex *all_dots_pos_color;

        const bgfx::ViewId main_view_id = 0;
        const uint32_t dot_color = 0xffffffff;
        const uint16_t dot_resolution = 10;
        const uint16_t grid_size = 100;
        const float m_radius = 0.1f;
        const float m_eye_z = 15.0f;
        float fov = 120;
        float bound = 0.0f;
        float y_ar = 0.0f;

        int32_t m_pt;
        bool m_r;
        bool m_g;
        bool m_b;
        bool m_a;

        // void makeDots(Dot *&dots, CircleVertex *&posColor)
        // {
        //     dots = new Dot[WIDTH_HEIGHT];
        //     posColor = new CircleVertex[WIDTH_HEIGHT];

        //     for (int y = 0; y < HEIGHT; y++)
        //     {
        //         for (int x = 0; x < WIDTH; x++)
        //         {
        //             dots[y * HEIGHT + x] = Dot(x, y, 0);
        //             uint32_t color = 0xff000000 +
        //                 (0x00ff0000 | (x % 0xff)) +
        //                 (0x0000ff00 | (y % 0xff)) +
        //                 (0x000000ff | (x + y % 0xff));
        //             posColor[y * HEIGHT + x] = CircleVertex{
        //                 (float)x,   // X co-ordinate
        //                 (float)y,   // Y co-ordinate
        //                 0.0f,       // Z co-ordinate
        //                 color  // ABGR Color
        //             };
        //         }
        //     }
        // }

        // void updateDots(Dot *dots, float time)
        // {
        //     for (int x = 0; x < HEIGHT; x++)
        //     {
        //         for (int y = 0; y < WIDTH; y++)
        //         {
        //             dots[x * WIDTH + y].update(time);
        //         }
        //     }
        // }

        // int getOperation(int dotIndex, Dot *dots)
        // {
        //     // igrore if dotIndex at the width or at height
        //     if (dots[dotIndex].x == WIDTH || dots[dotIndex].y == HEIGHT)
        //     {
        //         return -1;
        //     }
        //     // make the op bin flag for choosing the operation
        //     return (dots[dotIndex].value > 0.5f ? 1 : 0) +
        //            (dots[dotIndex + 1].value > 0.5f ? 2 : 0) +
        //            (dots[dotIndex + HEIGHT + 1].value > 0.5f ? 4 : 0) +
        //            (dots[dotIndex + HEIGHT].value > 0.5f ? 8 : 0);
        //     // vertical
        //     // horizontal
        //     // top left
        //     // top right
        //     // bottom left
        //     // bottom right
        //     // double vertical
        //     // double horizontal
        //     // double bottom left to top right
        //     // double bottom right to top left
        //     // Empty
        // }

        // void go()
        // {
        //     // update dot values
        //     updateDots(all_dots, 0.1f);
        //     // enter second loop for 2x2 dots
        //     for (int d = 0; d < WIDTH_HEIGHT; d++)
        //     {
        //         // calculate the operation for the local grid
        //         int operation = getOperation(d, all_dots);
        //         if (operation == -1)
        //             continue;
        //         // add operation to lines list
        //     }
        //     // draw all dot values
        //     // draw all lines
        // }
    };

} // namespace

ENTRY_IMPLEMENT_MAIN(
    Wavy, "Wavy", "My lil Wavy app", "https://google.com");

int _main_(int _argc, char **_argv) { return 0; };