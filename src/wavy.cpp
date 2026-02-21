#include "wavy.hpp"

bgfx::VertexLayout PosColor::ms_layout;

void PosColor::init() {
    ms_layout.begin()
        .add(bgfx::Attrib::Position, 2, bgfx::AttribType::Float)
        .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
        .end();
}

void createCircleVB(bgfx::VertexBufferHandle* _result, float radius, uint32_t color, uint16_t segments) {
    uint16_t  numVertices = segments + 1;
    uint32_t  size        = numVertices * sizeof(PosColor);
    PosColor* vertex      = new PosColor[numVertices];
    vertex[0]             = PosColor{0.0f, 0.0f, color};
    float angleStep       = 2.0f * M_PI / segments;
    float angle           = 0.0f;
    for (uint16_t i = 1; i < segments + 1; i++) {
        vertex[i] = PosColor{radius * cosf(angle), radius * sinf(angle), color};
        angle += angleStep;
    }
    *_result = bgfx::createVertexBuffer(bgfx::copy(vertex, size), PosColor::ms_layout);
    delete[] vertex;
}

void createCircleIB(bgfx::IndexBufferHandle* _result, uint16_t segments) {
    uint16_t  numIndices = segments * 3;
    uint32_t  size       = numIndices * sizeof(uint16_t);
    uint16_t* index      = new uint16_t[numIndices];
    int       i_offset   = 0;
    for (uint16_t i = 0; i < segments; i++) {
        index[i_offset]     = 0;
        index[i_offset + 1] = i + 1;
        index[i_offset + 2] = i + 2;
        i_offset += 3;
    }
    index[numIndices - 1] = 1;
    *_result              = bgfx::createIndexBuffer(bgfx::copy(index, size));
    delete[] index;
}

Wavy::Wavy(const char* _name, const char* _description, const char* _url)
      : entry::AppI(_name, _description, _url),
        noise(1738),
        main_view_id(0),
        state(0 | BGFX_STATE_WRITE_R | BGFX_STATE_WRITE_G | BGFX_STATE_WRITE_B | BGFX_STATE_WRITE_A |
              BGFX_STATE_WRITE_Z | BGFX_STATE_DEPTH_TEST_LESS | BGFX_STATE_CULL_CW | BGFX_STATE_MSAA),
        dot_color(0xffffffff),
        dot_resolution(10),
        time_mult(0.4f),
        grid_size(267),
        m_radius(0.1f),
        m_eye_z(15.0f),
        fov(120),
        bound(0.0f),
        y_ar(0.0f) {}

void Wavy::init(int32_t _argc, const char* const* _argv, uint32_t _width, uint32_t _height) {
    Args args(_argc, _argv);
    WIDTH      = _width;
    HEIGHT     = _height;
    old_width  = 0;
    old_height = 0;
    m_debug    = BGFX_DEBUG_TEXT;
    m_reset    = BGFX_RESET_VSYNC;
    std::cout << "width: " << _width << " height: " << _height << std::endl;
    y_ar = m_eye_z * float(HEIGHT) / float(WIDTH);
    bgfx::Init init;
    init.type              = args.m_type;
    init.vendorId          = args.m_pciId;
    init.platformData.nwh  = entry::getNativeWindowHandle(entry::kDefaultWindowHandle);
    init.platformData.ndt  = entry::getNativeDisplayHandle();
    init.platformData.type = entry::getNativeWindowHandleType();
    init.resolution.width  = WIDTH;
    init.resolution.height = HEIGHT;
    init.resolution.reset  = m_reset;
    std::cout << "Initializing bgfx..." << std::endl;
    if (!bgfx::init(init)) {
        std::cout << "bgfx::init FAILED" << std::endl;
        return;
    }
    std::cout << "bgfx initialized, renderer: " << bgfx::getRendererName(bgfx::getRendererType()) << std::endl;
    bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x0, 1.0f, 0);
    PosColor::init();
    makeDots(dots, &dot_count);
    createCircleVB(&m_vbh, m_radius, dot_color, dot_resolution);
    createCircleIB(&m_ibh, dot_resolution);
    std::cout << "Loading shaders..." << std::endl;
    m_program      = loadProgram("vs_wavy", "fs_wavy");
    std::cout << "vs_wavy/fs_wavy loaded, handle: " << m_program.idx << std::endl;
    m_program_inst = loadProgram("vs_wavy_inst", "fs_wavy");
    std::cout << "vs_wavy_inst/fs_wavy loaded, handle: " << m_program_inst.idx << std::endl;
    m_timeOffset   = bx::getHPCounter();
    imguiCreate();
    std::cout << "Init complete" << std::endl;
}

int Wavy::shutdown() {
    imguiDestroy();
    bgfx::destroy(m_ibh);
    bgfx::destroy(m_vbh);
    bgfx::destroy(m_program);
    bgfx::destroy(m_program_inst);
    delete[] dots;
    dots = nullptr;
    bgfx::shutdown();
    return 0;
}

bool Wavy::update() {
    if (!entry::processEvents(WIDTH, HEIGHT, m_debug, m_reset, &m_mouseState)) {
        imguiBeginFrame(m_mouseState.m_mx, m_mouseState.m_my,
                        (m_mouseState.m_buttons[entry::MouseButton::Left] ? IMGUI_MBUT_LEFT : 0) |
                            (m_mouseState.m_buttons[entry::MouseButton::Right] ? IMGUI_MBUT_RIGHT : 0) |
                            (m_mouseState.m_buttons[entry::MouseButton::Middle] ? IMGUI_MBUT_MIDDLE : 0),
                        m_mouseState.m_mz, uint16_t(WIDTH), uint16_t(HEIGHT));
        imguiEndFrame();

        if (old_width != WIDTH || old_height != HEIGHT)
            update_view();

        bgfx::touch(0);
        float time = (float)((bx::getHPCounter() - m_timeOffset) / double(bx::getHPFrequency())) * time_mult;

        updateDots(time);
        drawCirclesInstanced(state);
        drawLinesBatched(state);
        bgfx::frame();
        return true;
    }
    return false;
}

void Wavy::updateDots(float time) {
    for (int i = 0; i < dot_count; i++) {
        dots[i].update(time);
    }
}

void Wavy::drawCirclesInstanced(uint64_t state) {
    const uint16_t instanceStride = 80; // 64 (4x4 matrix) + 16 (vec4 instance data)
    uint32_t numInstances = (uint32_t)dot_count;
    uint32_t availInstances = bgfx::getAvailInstanceDataBuffer(numInstances, instanceStride);
    numInstances = (numInstances < availInstances) ? numInstances : availInstances;
    if (numInstances == 0) return;

    bgfx::InstanceDataBuffer idb;
    bgfx::allocInstanceDataBuffer(&idb, numInstances, instanceStride);

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
    bgfx::submit(main_view_id, m_program_inst);
}

void Wavy::drawLinesBatched(uint64_t state) {
    state |= BGFX_STATE_PT_LINES;

    const uint32_t maxVerticesPerBatch = 32766;
    const uint32_t lineColor = 0x7f7f7f7f; // pre-multiplied half-brightness white

    bgfx::TransientVertexBuffer tvb;
    bgfx::TransientIndexBuffer tib;
    PosColor* vertices = nullptr;
    uint16_t* indices = nullptr;
    uint32_t vi = 0;
    bool bufferAllocated = false;

    for (int i = 0; i < dot_count; i++) {
        int gx = i % x_count;
        int gy = i / x_count;
        if (gx >= x_count - 1 || gy >= y_count - 1)
            continue;

        float pos[8];
        getLinePos(i, pos);
        if (pos[0] == -1.0f) continue;

        int linesNeeded = (pos[4] != -1.0f) ? 2 : 1;
        uint32_t verticesNeeded = (uint32_t)(linesNeeded * 2);

        if (!bufferAllocated || vi + verticesNeeded > maxVerticesPerBatch) {
            if (bufferAllocated && vi > 0) {
                bgfx::setVertexBuffer(0, &tvb, 0, vi);
                bgfx::setIndexBuffer(&tib, 0, vi);
                bgfx::setState(state);
                bgfx::submit(main_view_id, m_program);
            }

            uint32_t avail = bgfx::getAvailTransientVertexBuffer(maxVerticesPerBatch, PosColor::ms_layout);
            if (avail < 2) break;
            uint32_t batchSize = (avail < maxVerticesPerBatch) ? avail : maxVerticesPerBatch;
            if (!bgfx::allocTransientBuffers(&tvb, PosColor::ms_layout, batchSize, &tib, batchSize)) {
                break;
            }
            vertices = (PosColor*)tvb.data;
            indices = (uint16_t*)tib.data;
            vi = 0;
            bufferAllocated = true;
        }

        vertices[vi] = PosColor{pos[0], pos[1], lineColor};
        indices[vi] = (uint16_t)vi;
        vi++;
        vertices[vi] = PosColor{pos[2], pos[3], lineColor};
        indices[vi] = (uint16_t)vi;
        vi++;

        if (linesNeeded == 2) {
            vertices[vi] = PosColor{pos[4], pos[5], lineColor};
            indices[vi] = (uint16_t)vi;
            vi++;
            vertices[vi] = PosColor{pos[6], pos[7], lineColor};
            indices[vi] = (uint16_t)vi;
            vi++;
        }
    }

    if (bufferAllocated && vi > 0) {
        bgfx::setVertexBuffer(0, &tvb, 0, vi);
        bgfx::setIndexBuffer(&tib, 0, vi);
        bgfx::setState(state);
        bgfx::submit(main_view_id, m_program);
    }
}

int Wavy::getOperation(int index) {
    return (dots[index].value >= 0.5f ? 1 : 0) + (dots[index + 1].value >= 0.5f ? 2 : 0) +
           (dots[index + x_count + 1].value >= 0.5f ? 4 : 0) + (dots[index + x_count].value >= 0.5f ? 8 : 0);
}

float Wavy::mid(const dot* d1, const dot* d2, bool x) const {
    float v1 = x ? d1->x : d1->y;
    float v2 = x ? d2->x : d2->y;
    float denom = d1->value + d2->value;
    if (denom < 1e-6f) return (v1 + v2) * 0.5f;
    return (v1 * d1->value + v2 * d2->value) / denom;
}

// Edge interpolation helpers for marching squares
float Wavy::edgeUpX(int index) { return mid(&dots[index], &dots[index + 1], true); }
float Wavy::edgeUpY(int index) { return dots[index].y; }
float Wavy::edgeDownX(int index) { return mid(&dots[index + x_count], &dots[index + x_count + 1], true); }
float Wavy::edgeDownY(int index) { return dots[index + x_count].y; }
float Wavy::edgeLeftX(int index) { return dots[index].x; }
float Wavy::edgeLeftY(int index) { return mid(&dots[index], &dots[index + x_count], false); }
float Wavy::edgeRightX(int index) { return dots[index + 1].x; }
float Wavy::edgeRightY(int index) { return mid(&dots[index + 1], &dots[index + x_count + 1], false); }

void Wavy::getLinePos(int index, float* pos) {
    int op = getOperation(index);
    pos[0] = -1;
    pos[1] = -1;
    pos[2] = -1;
    pos[3] = -1;
    pos[4] = -1;
    pos[5] = -1;
    pos[6] = -1;
    pos[7] = -1;
    switch (op) {
    case 1:
    case 14:
        pos[0] = edgeUpX(index);
        pos[1] = edgeUpY(index);
        pos[2] = edgeLeftX(index);
        pos[3] = edgeLeftY(index);
        break;
    case 2:
    case 13:
        pos[0] = edgeUpX(index);
        pos[1] = edgeUpY(index);
        pos[2] = edgeRightX(index);
        pos[3] = edgeRightY(index);
        break;
    case 4:
    case 11:
        pos[0] = edgeDownX(index);
        pos[1] = edgeDownY(index);
        pos[2] = edgeRightX(index);
        pos[3] = edgeRightY(index);
        break;
    case 8:
    case 7:
        pos[0] = edgeDownX(index);
        pos[1] = edgeDownY(index);
        pos[2] = edgeLeftX(index);
        pos[3] = edgeLeftY(index);
        break;
    case 3:
    case 12:
        pos[0] = edgeLeftX(index);
        pos[1] = edgeLeftY(index);
        pos[2] = edgeRightX(index);
        pos[3] = edgeRightY(index);
        break;
    case 6:
    case 9:
        pos[0] = edgeUpX(index);
        pos[1] = edgeUpY(index);
        pos[2] = edgeDownX(index);
        pos[3] = edgeDownY(index);
        break;
    case 5:
        pos[0] = edgeUpX(index);
        pos[1] = edgeUpY(index);
        pos[2] = edgeRightX(index);
        pos[3] = edgeRightY(index);
        pos[4] = edgeDownX(index);
        pos[5] = edgeDownY(index);
        pos[6] = edgeLeftX(index);
        pos[7] = edgeLeftY(index);
        break;
    case 10:
        pos[0] = edgeUpX(index);
        pos[1] = edgeUpY(index);
        pos[2] = edgeLeftX(index);
        pos[3] = edgeLeftY(index);
        pos[4] = edgeDownX(index);
        pos[5] = edgeDownY(index);
        pos[6] = edgeRightX(index);
        pos[7] = edgeRightY(index);
        break;
    default:
        break;
    }
}

void Wavy::makeDots(dot*& dots, int* dot_count) {
    x_count    = grid_size;
    y_count    = grid_size * HEIGHT / WIDTH;
    *dot_count = x_count * y_count;
    dots       = new dot[*dot_count];
    for (int y = 0; y < y_count; y++) {
        for (int x = 0; x < x_count; x++) {
            int i   = y * x_count + x;
            dots[i] = dot(getX(x), getY(y), float(x), float(y), 0.0f, &noise);
        }
    }
}

void Wavy::update_view() {
    old_width          = WIDTH;
    old_height         = HEIGHT;
    const bx::Vec3 at  = {0.0f, 0.0f, 0.0f};
    const bx::Vec3 eye = {0.0f, 0.0f, -m_eye_z};
    float          proj[16];
    float          view[16];
    float          inv_proj[16];
    float          world_pos[4] = {1.0f, 1.0f, 0.0f, 0.0f};
    bx::mtxLookAt(view, eye, at);
    bx::mtxProj(proj, fov, float(WIDTH) / float(HEIGHT), 0.1f, 100.0f, bgfx::getCaps()->homogeneousDepth);
    bgfx::setViewTransform(main_view_id, view, proj);
    bgfx::setViewRect(main_view_id, 0, 0, uint16_t(WIDTH), uint16_t(HEIGHT));
    bx::mtxInverse(inv_proj, proj);
    bx::mtxMul(world_pos, inv_proj, world_pos);
    bound = world_pos[0];
    updateDotLocations();
}

void Wavy::updateDotLocations() {
    for (int y = 0; y < y_count; y++) {
        for (int x = 0; x < x_count; x++) {
            int i     = y * x_count + x;
            dots[i].x = getX(x);
            dots[i].y = getY(y);
        }
    }
}

float Wavy::getX(int x) {
    return (bound * m_eye_z * (2 * (float(x) * (float(WIDTH) / float(x_count - 1))) - WIDTH)) / WIDTH;
}

float Wavy::getY(int y) {
    return (bound * y_ar * (HEIGHT - 2 * (float(y) * (float(HEIGHT) / float(y_count - 1))))) / HEIGHT;
}

ENTRY_IMPLEMENT_MAIN(Wavy, "Wavy", "My lil Wavy app", "https://google.com");

int _main_(int _argc, char** _argv) {
    return 0;
};