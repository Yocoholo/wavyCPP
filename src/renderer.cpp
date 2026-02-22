#include "renderer.hpp"
#include "marching_squares.hpp"
#include "helpers.h"
#include "bgfx_utils.h"
#include <bx/math.h>

// ── PosColor vertex layout ──────────────────────────────────────────────────

bgfx::VertexLayout PosColor::ms_layout;

void PosColor::init() {
    ms_layout.begin()
        .add(bgfx::Attrib::Position, 2, bgfx::AttribType::Float)
        .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
        .end();
}

// ── Unit quad mesh (for GPU SDF circle rendering) ───────────────────────────

static void createQuadVB(bgfx::VertexBufferHandle* result, uint32_t color) {
    // Unit quad: corners at (-1,-1) to (1,1)
    // The vertex shader scales by radius and translates to world position.
    // The fragment shader uses UV (-1..1) for SDF circle test.
    PosColor vertices[4] = {
        { -1.0f, -1.0f, color },  // bottom-left
        {  1.0f, -1.0f, color },  // bottom-right
        {  1.0f,  1.0f, color },  // top-right
        { -1.0f,  1.0f, color },  // top-left
    };
    *result = bgfx::createVertexBuffer(
        bgfx::copy(vertices, sizeof(vertices)), PosColor::ms_layout);
}

static void createQuadIB(bgfx::IndexBufferHandle* result) {
    uint16_t indices[6] = { 0, 1, 2, 0, 2, 3 };
    *result = bgfx::createIndexBuffer(bgfx::copy(indices, sizeof(indices)));
}

// ── Renderer ────────────────────────────────────────────────────────────────

Renderer::Renderer(bgfx::ViewId viewId, float radiusScale, uint32_t dotColor)
    : m_viewId(viewId), m_radiusScale(radiusScale), m_dotColor(dotColor) {
    m_program.idx     = bgfx::kInvalidHandle;
    m_programDot.idx  = bgfx::kInvalidHandle;
    m_vbh.idx         = bgfx::kInvalidHandle;
    m_ibh.idx         = bgfx::kInvalidHandle;
    m_instanceBuf.idx = bgfx::kInvalidHandle;
}

void Renderer::init() {
    PosColor::init();
    createQuadVB(&m_vbh, m_dotColor);
    createQuadIB(&m_ibh);

    // Instance data layout: one vec4 (x, y, brightness, radius) per dot
    m_instanceLayout.begin()
        .add(bgfx::Attrib::TexCoord7, 4, bgfx::AttribType::Float)
        .end();
    m_instanceBuf = bgfx::createDynamicVertexBuffer(
        1, m_instanceLayout, BGFX_BUFFER_ALLOW_RESIZE);

    debug_info("Loading shaders...");
    m_program     = loadProgram("vs_wavy", "fs_wavy");
    m_programDot  = loadProgram("vs_wavy_dot", "fs_wavy_dot");
    debug_info("Shaders loaded");
}

void Renderer::shutdown() {
    if (bgfx::isValid(m_instanceBuf)) bgfx::destroy(m_instanceBuf);
    if (bgfx::isValid(m_ibh))         bgfx::destroy(m_ibh);
    if (bgfx::isValid(m_vbh))         bgfx::destroy(m_vbh);
    if (bgfx::isValid(m_program))     bgfx::destroy(m_program);
    if (bgfx::isValid(m_programDot))  bgfx::destroy(m_programDot);
}

void Renderer::updateView(uint32_t width, uint32_t height, float eyeZ, float fov,
                          float yAR, float& outBound) {
    const bx::Vec3 at  = {0.0f, 0.0f, 0.0f};
    const bx::Vec3 eye = {0.0f, 0.0f, -eyeZ};
    float proj[16];
    float view[16];
    float inv_proj[16];
    float world_pos[4] = {1.0f, 1.0f, 0.0f, 0.0f};

    bx::mtxLookAt(view, eye, at);
    bx::mtxProj(proj, fov, float(width) / float(height), 0.1f, 100.0f,
                bgfx::getCaps()->homogeneousDepth);
    bgfx::setViewTransform(m_viewId, view, proj);
    bgfx::setViewRect(m_viewId, 0, 0, uint16_t(width), uint16_t(height));

    bx::mtxInverse(inv_proj, proj);
    bx::mtxMul(world_pos, inv_proj, world_pos);
    outBound = world_pos[0];
}

void Renderer::drawCirclesInstanced(const Grid& grid, uint64_t state) {
    // Remove face culling — 2D quads have no meaningful back-face
    state = (state & ~BGFX_STATE_CULL_MASK);

    const Dot* dots = grid.dots();
    uint32_t numDots = (uint32_t)grid.dotCount();
    if (numDots == 0) return;

    // Auto-scale radius from grid spacing so dots look right at any density
    float spacing = 2.0f * grid.bound() * grid.eyeZ() / float(grid.xCount() - 1);
    float radius  = spacing * m_radiusScale;

    // Fill the dynamic instance buffer (resizes automatically)
    const bgfx::Memory* mem = bgfx::alloc(numDots * 16);
    float* data = (float*)mem->data;
    for (uint32_t i = 0; i < numDots; i++) {
        const Dot& dot = dots[i];
        data[0] = dot.x;
        data[1] = dot.y;
        data[2] = dot.value;
        data[3] = radius;
        data += 4;
    }
    bgfx::update(m_instanceBuf, 0, mem);

    // Single draw call for all dots — no transient buffer limits
    bgfx::setVertexBuffer(0, m_vbh);
    bgfx::setIndexBuffer(m_ibh);
    bgfx::setInstanceDataBuffer(m_instanceBuf, 0, numDots);
    bgfx::setState(state);
    bgfx::submit(m_viewId, m_programDot);
}

void Renderer::drawLinesBatched(const Grid& grid, uint64_t state) {
    state |= BGFX_STATE_PT_LINES;

    const uint32_t maxVerticesPerBatch = 32766;
    const uint32_t lineColor = 0x7f7f7f7f;

    const Dot* dots = grid.dots();
    int dotCount    = grid.dotCount();
    int xCount      = grid.xCount();
    int yCount      = grid.yCount();

    bgfx::TransientVertexBuffer tvb;
    bgfx::TransientIndexBuffer  tib;
    PosColor* vertices = nullptr;
    uint16_t* indices  = nullptr;
    uint32_t  vi       = 0;
    bool allocated     = false;

    for (int i = 0; i < dotCount; i++) {
        int gx = i % xCount;
        int gy = i / xCount;
        if (gx >= xCount - 1 || gy >= yCount - 1)
            continue;

        float pos[8];
        MarchingSquares::getLinePos(dots, i, xCount, pos);
        if (pos[0] == -1.0f) continue;

        int linesNeeded = (pos[4] != -1.0f) ? 2 : 1;
        uint32_t verticesNeeded = (uint32_t)(linesNeeded * 2);

        if (!allocated || vi + verticesNeeded > maxVerticesPerBatch) {
            if (allocated && vi > 0) {
                bgfx::setVertexBuffer(0, &tvb, 0, vi);
                bgfx::setIndexBuffer(&tib, 0, vi);
                bgfx::setState(state);
                bgfx::submit(m_viewId, m_program);
            }

            uint32_t avail = bgfx::getAvailTransientVertexBuffer(maxVerticesPerBatch, PosColor::ms_layout);
            if (avail < 2) break;
            uint32_t batchSize = (avail < maxVerticesPerBatch) ? avail : maxVerticesPerBatch;
            if (!bgfx::allocTransientBuffers(&tvb, PosColor::ms_layout, batchSize, &tib, batchSize))
                break;

            vertices = (PosColor*)tvb.data;
            indices  = (uint16_t*)tib.data;
            vi       = 0;
            allocated = true;
        }

        vertices[vi] = PosColor{pos[0], pos[1], lineColor};
        indices[vi]  = (uint16_t)vi;
        vi++;
        vertices[vi] = PosColor{pos[2], pos[3], lineColor};
        indices[vi]  = (uint16_t)vi;
        vi++;

        if (linesNeeded == 2) {
            vertices[vi] = PosColor{pos[4], pos[5], lineColor};
            indices[vi]  = (uint16_t)vi;
            vi++;
            vertices[vi] = PosColor{pos[6], pos[7], lineColor};
            indices[vi]  = (uint16_t)vi;
            vi++;
        }
    }

    if (allocated && vi > 0) {
        bgfx::setVertexBuffer(0, &tvb, 0, vi);
        bgfx::setIndexBuffer(&tib, 0, vi);
        bgfx::setState(state);
        bgfx::submit(m_viewId, m_program);
    }
}
