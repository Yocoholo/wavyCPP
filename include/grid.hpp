#pragma once

#include "OpenSimplex.hpp"
#include <cstdint>

struct Dot {
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
        : x(x), y(y), s_x(s_x), s_y(s_y), value(value),
          noise(noise), simplex_X(s_x / 20), simplex_Y(s_y / 20) {}

    void update(float time) {
        double res = noise->noise3_XYBeforeZ(simplex_X, simplex_Y, time);
        res = (res + 1) / 2;
        value = float(res);
    }
};

class Grid {
public:
    Grid(uint16_t gridSize, float eyeZ, float fov);
    ~Grid();

    void create(uint32_t width, uint32_t height);
    void updateDots(float time);
    void updateLayout(uint32_t width, uint32_t height, float bound);

    Dot*     dots()      const { return m_dots; }
    int      dotCount()  const { return m_dotCount; }
    int      xCount()    const { return m_xCount; }
    int      yCount()    const { return m_yCount; }
    float    bound()     const { return m_bound; }
    float    eyeZ()      const { return m_eyeZ; }
    float    fov()       const { return m_fov; }
    float    yAR()       const { return m_yAR; }

    float getX(int x) const;
    float getY(int y) const;

private:
    OpenSimplex m_noise;
    Dot*        m_dots     = nullptr;
    int         m_dotCount = 0;
    int         m_xCount   = 0;
    int         m_yCount   = 0;
    uint16_t    m_gridSize;
    float       m_eyeZ;
    float       m_fov;
    float       m_bound    = 0.0f;
    float       m_yAR      = 0.0f;
    uint32_t    m_width    = 0;
    uint32_t    m_height   = 0;
};
