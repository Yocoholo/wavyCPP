#pragma once

#include "helpers.h"
#include "grid.hpp"
#include "renderer.hpp"
#include "common.h"
#include "imgui/imgui.h"
#include <bx/uint32_t.h>

class Wavy : public entry::AppI {
public:
    Wavy(const char* _name, const char* _description, const char* _url);

    void init(int32_t _argc, const char* const* _argv,
              uint32_t _width, uint32_t _height) override;
    int  shutdown() override;
    bool update() override;

private:
    void onResize();

    entry::MouseState m_mouseState;
    uint32_t m_debug;
    uint32_t m_reset;
    uint32_t m_width;
    uint32_t m_height;
    uint32_t m_oldWidth;
    uint32_t m_oldHeight;
    int64_t  m_timeOffset;
    float    m_timeMult;

    const bgfx::ViewId m_viewId;
    const uint64_t     m_state;

    Grid     m_grid;
    Renderer m_renderer;
};
