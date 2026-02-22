#pragma once

#include "grid.hpp"
#include <bgfx/bgfx.h>
#include <cmath>

struct PosColor {
    float    m_x;
    float    m_y;
    uint32_t m_abgr;

    static void init();
    static bgfx::VertexLayout ms_layout;
};

class Renderer {
public:
    Renderer(bgfx::ViewId viewId, float radius, uint32_t dotColor, uint16_t dotSegments);

    void init();
    void shutdown();

    void drawCirclesInstanced(const Grid& grid, uint64_t state);
    void drawLinesBatched(const Grid& grid, uint64_t state);

    void updateView(uint32_t width, uint32_t height, float eyeZ, float fov,
                    float yAR, float& outBound);

    bgfx::ProgramHandle lineProgram()     const { return m_program; }
    bgfx::ProgramHandle instanceProgram() const { return m_programInst; }

private:
    bgfx::ViewId             m_viewId;
    bgfx::ProgramHandle      m_program;
    bgfx::ProgramHandle      m_programInst;
    bgfx::VertexBufferHandle m_vbh;
    bgfx::IndexBufferHandle  m_ibh;

    float    m_radius;
    uint32_t m_dotColor;
    uint16_t m_dotSegments;
};
