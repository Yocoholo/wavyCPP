#include <helpers.cpp>
#include <dot.cpp>
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
    struct PosColor
    {
        float m_x;
        float m_y;
        uint32_t m_abgr;

        static void init()
        {
            ms_layout
                .begin()
                .add(bgfx::Attrib::Position, 2, bgfx::AttribType::Float)
                .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
                .end();
        };

        static bgfx::VertexLayout ms_layout;
    };
    bgfx::VertexLayout PosColor::ms_layout;

    // Create a vertex buffer for the circle
    static void createCircleVB(bgfx::VertexBufferHandle *_result, float radius, uint32_t color, uint16_t segments)
    {
        // Calculate the number of vertices and the size of the buffer
        uint16_t numVertices = segments + 1;
        uint32_t size = numVertices * sizeof(PosColor);

        // Get a pointer to the vertex data
        PosColor *vertex = new PosColor[numVertices];

        // Add the center vertex
        vertex[0] = PosColor{
            0.0f,
            0.0f,
            color};

        // Add the perimeter vertices
        float angleStep = 2.0f * M_PI / segments;
        float angle = 0.0f;
        for (uint16_t i = 1; i < segments + 1; i++)
        {
            vertex[i] = PosColor{
                radius * cosf(angle),
                radius * sinf(angle),
                color};
            angle += angleStep;
        }
        // Return the vertex buffer handle
        *_result = bgfx::createVertexBuffer(bgfx::copy(vertex, size), PosColor::ms_layout);
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
        // Return the transient index buffer
        *_result = bgfx::createIndexBuffer(bgfx::copy(index, size));
    }

    static void createLineVertices(PosColor*& vertices, uint16_t verticesCount, float startX, float startY, float endX, float endY, float m_line_thickness, uint16_t segments, uint32_t color)
    {
        float angle = atan2f(endY - startY,  endX - startX);
        float angle_step = M_PI / segments;
        float start_angle = angle + M_PI_2;
        float end_angle = angle - M_PI_2;

        for (int i = 0; i < segments*2; i+=2){
            vertices[i] = PosColor{
                startX + m_line_thickness * cosf(start_angle),
                startY + m_line_thickness * sinf(start_angle),
                color
            };
            vertices[i +1] = PosColor{
                endX + m_line_thickness * cosf(end_angle),
                endY + m_line_thickness * sinf(end_angle),
                color
            };
            start_angle += angle_step;
            end_angle += angle_step;
        }
    }

    static void createLineIndicies(uint16_t*& indicies,uint16_t& indiciesCount, uint16_t verticesCount){
        indiciesCount = verticesCount +1;
        int oddIndex = 0;
        int evenIndex = (verticesCount) / 2;

        for (int i = 0; i < verticesCount; ++i) {
            if (i % 2 != 0) {
                indicies[oddIndex] = i; // Store odd numbers in the array
                oddIndex++;
            } else {
                indicies[evenIndex] = i; // Store even numbers in the array
                evenIndex++;
            }
        }
        indicies[verticesCount] = 1; // complete the line loop
    }

    class Wavy : public entry::AppI
    {
    public:
        Wavy(const char *_name, const char *_description, const char *_url)
            : entry::AppI(_name, _description, _url), m_pt(0), m_r(true), m_g(true), m_b(true), m_a(true)
        {
        }

        void init(int32_t _argc, const char *const *_argv, uint32_t _width, uint32_t _height) override
        {
            Args args(_argc, _argv);

            WIDTH = _width;
            HEIGHT = _height;
            old_width = 0;
            old_height = 0;
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
            //bgfx::setDebug(m_debug);

            // Create vertex stream declaration.

            // Set view 0 clear state.
            // bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x303030ff, 1.0f, 0);
            bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x0, 1.0f, 0);

            // Create vertex stream declaration.
            PosColor::init();

            // Create all of the dots
            makeDots(dots, &dot_count);

            // Create a vertex buffer and index buffer for the circle
            createCircleVB(&m_vbh, m_radius, dot_color, dot_resolution);
            createCircleIB(&m_ibh, dot_resolution);

            // create uniform for modifying dot brightness
            u_brightness = bgfx::createUniform("u_brightness", bgfx::UniformType::Vec4);

            // Create program from shaders.
            m_program = loadProgram("vs_wavy", "fs_wavy");
            //m_program = loadProgram("vs_cubes", "fs_cubes");

            m_timeOffset = bx::getHPCounter();
            imguiCreate();

        }

        virtual int shutdown() override
        {
            imguiDestroy();
            bgfx::destroy(u_brightness);
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
                // showExampleDialog(this);
                imguiEndFrame();

                if (old_width != WIDTH || old_height != HEIGHT) update_view();

                // This dummy draw call is here to make sure that view 0 is cleared
                // if no other draw calls are submitted to view 0.
                bgfx::touch(0);
                float time = (float)((bx::getHPCounter() - m_timeOffset) / double(bx::getHPFrequency()))* time_mult;

                drawCircles(time, state);
                drawLines(state);
                // Advance to next frame. Rendering thread will be kicked to
                // process submitted rendering primitives.
                bgfx::frame();
                return true;
            }
            return false;
        }

        // Draw a filled circle with the given parameters
        void drawCircles(float time, uint64_t state)
        {
            state |= UINT64_C(0);
            for (int i = 0; i < dot_count; i++)
            {
                drawCircle(&dots[i], time, state);
            }
        }

        void drawCircle(dot* dot, float time, uint64_t state){
            dot->update(time);
            float brightness[16] = {dot->value, 0.0f, 0.0f, 0.0f};
            bgfx::setUniform(u_brightness, brightness);

            float mtx[16];
            bx::mtxTranslate(mtx, dot->x, dot->y, 0.0f);

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

        void drawLines(uint64_t state){

            state |= BGFX_STATE_PT_LINES;
            for (int i = 0; i < dot_count; i++)
            {
                // skip the last line and the last column
                if (dots[i].s_x == x_count-1 || dots[i].s_y == y_count-1) continue;
                float pos[8];
                getLinePos(i, pos);
                // skip if the current dot space has no lines to draw
                if (pos[0] == -1) continue;
                drawLine(state, pos[0], pos[1], pos[2], pos[3]);
                if (pos[4] == -1) continue;
                drawLine(state, pos[4], pos[5], pos[6], pos[7]);
            }
        }

        void drawLine(uint64_t state, float startX, float startY, float endX, float endY){

            bgfx::TransientVertexBuffer tvb;
            bgfx::TransientIndexBuffer tib;

            PosColor* vertices;
            uint16_t* indicies;
            uint16_t verticesCount = 2;
            uint16_t indiciesCount = 2;
            bgfx::allocTransientBuffers(&tvb, PosColor::ms_layout, verticesCount, &tib, indiciesCount);
            vertices = (PosColor*)tvb.data;
            indicies = (uint16_t*)tib.data;

            vertices[0].m_x = startX;
            vertices[0].m_y = startY;
            vertices[0].m_abgr = dot_color;
            vertices[1].m_x = endX;
            vertices[1].m_y = endY;
            vertices[1].m_abgr = dot_color;

            indicies[0] = 0;
            indicies[1] = 1;

            float brightness[16] = {1.0f, 0.0f, 0.0f, 0.0f};
            bgfx::setUniform(u_brightness, brightness);

            // Set vertex and index buffer.
            bgfx::setVertexBuffer(0, &tvb);
            bgfx::setIndexBuffer(&tib);

            // Set render states.
            bgfx::setState(state);

            // Submit primitive for rendering to view 0.
            bgfx::submit(main_view_id, m_program);
        }


        int getOperation(int index){
            // make the op bin flag for choosing the operation
           return (dots[index].value                >= 0.5f ? 1 : 0)
                + (dots[index +1].value             >= 0.5f ? 2 : 0)
                + (dots[index + x_count +1].value   >= 0.5f ? 4 : 0)
                + (dots[index + x_count].value      >= 0.5f ? 8 : 0);
        }

        float mid(dot* d1, dot* d2, bool x){
            // Offset v1 to avoid midpoint being 0 and causing a divide by 0 issue.
            float v1 = x ? d1->x : d1->y;
            float v2 = x ? d2->x : d2->y;
            return (v1*d1->value + v2*d2->value)/(d1->value + d2->value);
        }

        #define upX mid(&dots[index], &dots[index + 1], true);
        #define upY dots[index].y;
        #define downX mid(&dots[index + x_count], &dots[index + x_count +1], true);
        #define downY dots[index + x_count].y;
        #define leftX dots[index].x;
        #define leftY mid(&dots[index], &dots[index + x_count], false);
        #define rightX dots[index +1].x;
        #define rightY mid(&dots[index + 1], &dots[index + x_count +1], false);

        void getLinePos(int index, float *pos){
            int op = getOperation(index);
            pos[0] = -1; pos[1] = -1; pos[2] = -1; pos[3] = -1; pos[4] = -1; pos[5] = -1; pos[6] = -1; pos[7] = -1;
            switch (op) {
                case 1:
                case 14:    // up, left
                    pos[0] = upX;
                    pos[1] = upY;
                    pos[2] = leftX;
                    pos[3] = leftY;
                    break;
                case 2:
                case 13:    // up, right
                    pos[0] = upX;
                    pos[1] = upY;
                    pos[2] = rightX;
                    pos[3] = rightY;
                    break;
                case 4:
                case 11:    // down, right
                    pos[0] = downX;
                    pos[1] = downY;
                    pos[2] = rightX;
                    pos[3] = rightY;
                    break;
                case 8:
                case 7:     // down, left
                    pos[0] = downX;
                    pos[1] = downY;
                    pos[2] = leftX;
                    pos[3] = leftY;
                    break;
                case 3:
                case 12:    // left, right
                    pos[0] = leftX;
                    pos[1] = leftY;
                    pos[2] = rightX;
                    pos[3] = rightY;
                    break;
                case 6:
                case 9:     // up, down
                    pos[0] = upX;
                    pos[1] = upY;
                    pos[2] = downX;
                    pos[3] = downY;
                    break;
                case 5:     // up, right, down, left
                    pos[0] = upX;
                    pos[1] = upY;
                    pos[2] = rightX
                    pos[3] = rightY
                    pos[4] = downX;
                    pos[5] = downY;
                    pos[6] = leftX;
                    pos[7] = leftY;
                    break;
                case 10:    // up, left, down, right
                    pos[0] = upX;
                    pos[1] = upY;
                    pos[2] = leftX;
                    pos[3] = leftY;
                    pos[4] = downX;
                    pos[5] = downY;
                    pos[6] = rightX;
                    pos[7] = rightY;
                    break;
                default:
                    break;
            }
        }

        void makeDots(dot *&dots, int *dot_count)
        {
            x_count = grid_size;
            y_count = grid_size * HEIGHT / WIDTH;
            *dot_count = x_count * y_count;
            dots = new dot[*dot_count];

            for (int y = 0; y < y_count; y++)
            {
                for (int x = 0; x < x_count; x++)
                {
                    int i = y * x_count + x;
                    // convert x,y from world space to screen space coordinates
                    dots[i] = dot(getX(x), getY(y),float(x), float(y), 0.0f, &noise);
                }
            }

        }

        void update_view()
        {
            old_width = WIDTH;
            old_height = HEIGHT;

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
            updateDotLocations();
        }

        void updateDotLocations(){
            for (int y = 0; y < y_count; y++)
            {
                for (int x = 0; x < x_count; x++)
                {
                    int i = y * x_count + x;
                    dots[i].x = getX(x);
                    dots[i].y = getY(y);
                }
            }
        }

        float getX(int x){
            return (bound * m_eye_z * ( 2*( float(x) * (float(WIDTH) / float(x_count-1))) - WIDTH)) / WIDTH;
        }

        float getY(int y){
            return (bound * y_ar * (HEIGHT - 2*(float(y) * (float(HEIGHT)/float(y_count-1))))) / HEIGHT;
        }

        entry::MouseState m_mouseState;

        uint32_t m_debug;
        uint32_t m_reset;
        bgfx::ProgramHandle m_program;
        bgfx::VertexBufferHandle m_vbh;
        bgfx::IndexBufferHandle m_ibh;

        uint32_t WIDTH;
        uint32_t HEIGHT;
        uint32_t old_width;
        uint32_t old_height;
        int64_t m_timeOffset;
        PosColor *all_dots_pos_color;
        OpenSimplex noise = OpenSimplex(1738);
        dot* dots;
        bgfx::UniformHandle u_brightness;

        const bgfx::ViewId main_view_id = 0;
        const uint64_t state = 0 | BGFX_STATE_WRITE_R | BGFX_STATE_WRITE_G | BGFX_STATE_WRITE_B | BGFX_STATE_WRITE_A | BGFX_STATE_WRITE_Z | BGFX_STATE_DEPTH_TEST_LESS | BGFX_STATE_CULL_CW | BGFX_STATE_MSAA;
        const uint32_t dot_color = 0xffffffff;
        const uint16_t dot_resolution = 10;
        float time_mult = 0.4f;
        int dot_count;
        int x_count;
        int y_count;
        const uint16_t grid_size = 267;
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

    };

} // namespace

ENTRY_IMPLEMENT_MAIN(
    Wavy, "Wavy", "My lil Wavy app", "https://google.com");

int _main_(int _argc, char **_argv) { return 0; };