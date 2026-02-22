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

// ── Circle mesh helpers ─────────────────────────────────────────────────────

static void createCircleVB(bgfx::VertexBufferHandle* result, float radius,
                           uint32_t color, uint16_t segments) {
    uint16_t  numVertices = segments + 1;
    uint32_t  size        = numVertices * sizeof(PosColor);
    PosColor* vertex      = new PosColor[numVertices];
    vertex[0]             = PosColor{0.0f, 0.0f, color};
    float angleStep       = 2.0f * float(M_PI) / segments;
    float angle           = 0.0f;
    for (uint16_t i = 1; i <= segments; i++) {
        vertex[i] = PosColor{radius * cosf(angle), radius * sinf(angle), color};
        angle += angleStep;
    }
    *result = bgfx::createVertexBuffer(bgfx::copy(vertex, size), PosColor::ms_layout);
    delete[] vertex;
}

static void createCircleIB(bgfx::IndexBufferHandle* result, uint16_t segments) {
    uint16_t  numIndices = segments * 3;
    uint32_t  size       = numIndices * sizeof(uint16_t);
    uint16_t* index      = new uint16_t[numIndices];
    int       offset     = 0;
    for (uint16_t i = 0; i < segments; i++) {
        index[offset]     = 0;
        index[offset + 1] = i + 1;
        index[offset + 2] = i + 2;
        offset += 3;
    }
    index[numIndices - 1] = 1;
    *result = bgfx::createIndexBuffer(bgfx::copy(index, size));
    delete[] index;
}

// ── Renderer ────────────────────────────────────────────────────────────────

Renderer::Renderer(bgfx::ViewId viewId, float radius, uint32_t dotColor, uint16_t dotSegments)
    : m_viewId(viewId), m_radius(radius), m_dotColor(dotColor), m_dotSegments(dotSegments) {
    m_program.idx     = bgfx::kInvalidHandle;
    m_programInst.idx = bgfx::kInvalidHandle;
    m_vbh.idx         = bgfx::kInvalidHandle;
    m_ibh.idx         = bgfx::kInvalidHandle;
}

void Renderer::init() {
    PosColor::init();
    createCircleVB(&m_vbh, m_radius, m_dotColor, m_dotSegments);
    createCircleIB(&m_ibh, m_dotSegments);

    debug_info("Loading shaders...");
    m_program     = loadProgram("vs_wavy", "fs_wavy");
    m_programInst = loadProgram("vs_wavy_inst", "fs_wavy");
    debug_info("Shaders loaded");
}

void Renderer::shutdown() {
    if (bgfx::isValid(m_ibh))         bgfx::destroy(m_ibh);
    if (bgfx::isValid(m_vbh))         bgfx::destroy(m_vbh);
    if (bgfx::isValid(m_program))     bgfx::destroy(m_program);
    if (bgfx::isValid(m_programInst)) bgfx::destroy(m_programInst);
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
    // Remove face culling — 2D circles have no meaningful back-face
    state = (state & ~BGFX_STATE_CULL_MASK);

    const uint16_t instanceStride = 80; // 64 (4x4 matrix) + 16 (vec4 instance data)
    uint32_t numInstances = (uint32_t)grid.dotCount();
    uint32_t avail = bgfx::getAvailInstanceDataBuffer(numInstances, instanceStride);
    numInstances = (numInstances < avail) ? numInstances : avail;
    if (numInstances == 0) return;

    bgfx::InstanceDataBuffer idb;
    bgfx::allocInstanceDataBuffer(&idb, numInstances, instanceStride);

    const Dot* dots = grid.dots();
    uint8_t* data = idb.data;
    for (uint32_t i = 0; i < numInstances; i++) {
        float* mtx = (float*)data;
        bx::mtxTranslate(mtx, dots[i].x, dots[i].y, 0.0f);

        float* extra = (float*)&data[64];
        extra[0] = dots[i].value;
        extra[1] = 0.0f;
        extra[2] = 0.0f;
        extra[3] = 0.0f;

        data += instanceStride;
    }

    bgfx::setVertexBuffer(0, m_vbh);
    bgfx::setIndexBuffer(m_ibh);
    bgfx::setInstanceDataBuffer(&idb);
    bgfx::setState(state);
    bgfx::submit(m_viewId, m_programInst);
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
