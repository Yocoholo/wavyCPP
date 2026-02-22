#include "main.hpp"
#include "bgfx_utils.h"
#include <bx/timer.h>

#define NUM_DOTS 1000
#define ANIMATION_SPEED 0.4f
#define EYE_Z 15.0f
#define FOV 120.0f

Wavy::Wavy(const char* _name, const char* _description, const char* _url)
    : entry::AppI(_name, _description, _url),
      m_viewId(0),
      m_state(0
          | BGFX_STATE_WRITE_R | BGFX_STATE_WRITE_G
          | BGFX_STATE_WRITE_B | BGFX_STATE_WRITE_A
          | BGFX_STATE_WRITE_Z | BGFX_STATE_DEPTH_TEST_LESS
          | BGFX_STATE_CULL_CW | BGFX_STATE_MSAA),
      m_timeMult(ANIMATION_SPEED),
      m_grid(NUM_DOTS, EYE_Z, FOV),
      m_renderer(0, 0.3f, 0xffffffff) {}

void Wavy::init(int32_t _argc, const char* const* _argv,
                uint32_t _width, uint32_t _height) {
    init_log_level();
    Args args(_argc, _argv);

    m_width     = _width;
    m_height    = _height;
    m_oldWidth  = 0;
    m_oldHeight = 0;
    m_debug     = BGFX_DEBUG_TEXT;
    m_reset     = BGFX_RESET_VSYNC;

    debug_info("width: %u height: %u", _width, _height);

    // Init bgfx
    bgfx::Init init;
    init.type              = args.m_type;
    init.vendorId          = args.m_pciId;
    init.platformData.nwh  = entry::getNativeWindowHandle(entry::kDefaultWindowHandle);
    init.platformData.ndt  = entry::getNativeDisplayHandle();
    init.platformData.type = entry::getNativeWindowHandleType();
    init.resolution.width  = m_width;
    init.resolution.height = m_height;
    init.resolution.reset  = m_reset;

    debug_info("Initializing bgfx...");
    if (!bgfx::init(init)) {
        debug_error("bgfx::init failed");
        return;
    }
    debug_info("bgfx initialized, renderer: %s",
               bgfx::getRendererName(bgfx::getRendererType()));

    bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x0, 1.0f, 0);

    // Init subsystems
    m_grid.create(m_width, m_height);
    m_renderer.init();

    m_timeOffset = bx::getHPCounter();
    imguiCreate();
    debug_info("Initialization complete");
}

int Wavy::shutdown() {
    imguiDestroy();
    m_renderer.shutdown();
    // Grid cleans up in its destructor
    bgfx::shutdown();
    return 0;
}

bool Wavy::update() {
    if (!entry::processEvents(m_width, m_height, m_debug, m_reset, &m_mouseState)) {
        imguiBeginFrame(m_mouseState.m_mx, m_mouseState.m_my,
            (m_mouseState.m_buttons[entry::MouseButton::Left]   ? IMGUI_MBUT_LEFT   : 0) |
            (m_mouseState.m_buttons[entry::MouseButton::Right]  ? IMGUI_MBUT_RIGHT  : 0) |
            (m_mouseState.m_buttons[entry::MouseButton::Middle] ? IMGUI_MBUT_MIDDLE : 0),
            m_mouseState.m_mz, uint16_t(m_width), uint16_t(m_height));
        imguiEndFrame();

        if (m_oldWidth != m_width || m_oldHeight != m_height)
            onResize();

        bgfx::touch(0);

        float time = (float)((bx::getHPCounter() - m_timeOffset)
                     / double(bx::getHPFrequency())) * m_timeMult;

        m_grid.updateDots(time);
        m_renderer.drawCirclesInstanced(m_grid, m_state);
        m_renderer.drawLinesBatched(m_grid, m_state);
        bgfx::frame();
        return true;
    }
    return false;
}

void Wavy::onResize() {
    m_oldWidth  = m_width;
    m_oldHeight = m_height;

    float bound = 0.0f;
    m_renderer.updateView(m_width, m_height, m_grid.eyeZ(), m_grid.fov(),
                          m_grid.yAR(), bound);
    m_grid.updateLayout(m_width, m_height, bound);
}

ENTRY_IMPLEMENT_MAIN(Wavy, "Wavy", "My lil Wavy app", "https://google.com");

int _main_(int _argc, char** _argv) {
    return 0;
};
