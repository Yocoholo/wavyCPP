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
    Renderer(bgfx::ViewId viewId, float radiusScale, uint32_t dotColor);

    void init();
    void shutdown();

    void drawCirclesInstanced(const Grid& grid, uint64_t state);
    void drawLinesBatched(const Grid& grid, uint64_t state);

    void updateView(uint32_t width, uint32_t height, float eyeZ, float fov,
                    float yAR, float& outBound);

    bgfx::ProgramHandle lineProgram() const { return m_program; }
    bgfx::ProgramHandle dotProgram()  const { return m_programDot; }

private:
    bgfx::ViewId                    m_viewId;
    bgfx::ProgramHandle             m_program;
    bgfx::ProgramHandle             m_programDot;
    bgfx::VertexBufferHandle        m_vbh;
    bgfx::IndexBufferHandle         m_ibh;
    bgfx::DynamicVertexBufferHandle m_instanceBuf;
    bgfx::VertexLayout              m_instanceLayout;

    float    m_radiusScale;  // fraction of dot spacing
    uint32_t m_dotColor;
};
