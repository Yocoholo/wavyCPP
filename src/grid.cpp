#include "grid.hpp"

Grid::Grid(uint16_t gridSize, float eyeZ, float fov)
    : m_noise(1738), m_gridSize(gridSize), m_eyeZ(eyeZ), m_fov(fov) {}

Grid::~Grid() {
    delete[] m_dots;
    m_dots = nullptr;
}

void Grid::create(uint32_t width, uint32_t height) {
    m_width  = width;
    m_height = height;
    m_yAR    = m_eyeZ * float(height) / float(width);
    m_xCount = m_gridSize;
    m_yCount = m_gridSize * height / width;
    m_dotCount = m_xCount * m_yCount;

    delete[] m_dots;
    m_dots = new Dot[m_dotCount];

    for (int y = 0; y < m_yCount; y++) {
        for (int x = 0; x < m_xCount; x++) {
            int i = y * m_xCount + x;
            m_dots[i] = Dot(getX(x), getY(y), float(x), float(y), 0.0f, &m_noise);
        }
    }
}

void Grid::updateDots(float time) {
    for (int i = 0; i < m_dotCount; i++) {
        m_dots[i].update(time);
    }
}

void Grid::updateLayout(uint32_t width, uint32_t height, float bound) {
    m_width  = width;
    m_height = height;
    m_bound  = bound;
    m_yAR    = m_eyeZ * float(height) / float(width);

    for (int y = 0; y < m_yCount; y++) {
        for (int x = 0; x < m_xCount; x++) {
            int i = y * m_xCount + x;
            m_dots[i].x = getX(x);
            m_dots[i].y = getY(y);
        }
    }
}

float Grid::getX(int x) const {
    return (m_bound * m_eyeZ * (2 * (float(x) * (float(m_width) / float(m_xCount - 1))) - m_width)) / m_width;
}

float Grid::getY(int y) const {
    return (m_bound * m_yAR * (m_height - 2 * (float(y) * (float(m_height) / float(m_yCount - 1))))) / m_height;
}
