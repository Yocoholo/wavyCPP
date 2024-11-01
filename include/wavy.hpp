#pragma once

#include <helpers.cpp>
#include <string>
#include <iostream>
#include <conio.h>
#include "common.h"
#include "bgfx_utils.h"
#include "imgui/imgui.h"
#include <OpenSimplex.cpp>
#include <bx/uint32_t.h>
#include <math.h>

namespace wavy {
class Wavy : public entry::AppI {
public:
    struct PosColor
    {
        float m_x;
        float m_y;
        uint32_t m_abgr;

        void init()
        {
            ms_layout
                .begin()
                .add(bgfx::Attrib::Position, 2, bgfx::AttribType::Float)
                .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
                .end();
        };

        bgfx::VertexLayout ms_layout;
    };

    struct Dot
    {
        float x;
        float y;
        float s_x;
        float s_y;
        float value;
        float simplex_X;
        float simplex_Y;
        OpenSimplex* noise;

        Dot() = default;
        Dot(float x, float y, float s_x, float s_y, float value, OpenSimplex* noise)
            : x(x), y(y), s_x(s_x), s_y(s_y), value(value), noise(noise)
        {
            simplex_X = s_x / 20;
            simplex_Y = s_y / 20;
        }

        void update(float time)
        {
            double res = noise->noise3_XYBeforeZ(simplex_X, simplex_Y, time);
            res = (res + 1) / 2;
            value = static_cast<float>(res);
        }
    };

    bgfx::VertexLayout PosColor::ms_layout;

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
    Dot* dots;
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

    Wavy(const char *_name, const char *_description, const char *_url) :
        entry::AppI(_name, _description, _url), m_pt(0), m_r(true), m_g(true), m_b(true), m_a(true) {}
    ~Wavy() noexcept override = default;

    void init(int32_t _argc, const char *const *_argv, uint32_t _width, uint32_t _height) override;
    void drawCircles(float time, uint64_t state);
    void drawCircle(Dot* dot, float time, uint64_t state);
    void drawLines(uint64_t state);
    void drawLine(uint64_t state, float startX, float startY, float endX, float endY);
    void updateDotLocations();
    void getLinePos(int index, float *pos);
    void makeDots(Dot *&dots, int *dot_count);
    void update_view();
    void createCircleVB(bgfx::VertexBufferHandle *_result, float radius, uint32_t color, uint16_t segments);
    void createCircleIB(bgfx::IndexBufferHandle *_result, uint16_t segments);
    void createLineVertices(PosColor*& vertices, uint16_t verticesCount, float startX, float startY, float endX, float endY, float m_line_thickness, uint16_t segments, uint32_t color);
    void createLineIndicies(uint16_t*& indicies,uint16_t& indiciesCount, uint16_t verticesCount);
    virtual int shutdown() override;
    bool update() override;
    int getOperation(int index);
    float mid(Dot* d1, Dot* d2, bool x);
    float getX(int x);
    float getY(int y);

    #define upX mid(&dots[index], &dots[index + 1], true);
    #define upY dots[index].y;
    #define downX mid(&dots[index + x_count], &dots[index + x_count +1], true);
    #define downY dots[index + x_count].y;
    #define leftX dots[index].x;
    #define leftY mid(&dots[index], &dots[index + x_count], false);
    #define rightX dots[index +1].x;
    #define rightY mid(&dots[index + 1], &dots[index + x_count +1], false);

};
}