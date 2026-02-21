#pragma once

#include "helpers.h"
#include <string>
#include <iostream>
#include "common.h"
#include "bgfx_utils.h"
#include "imgui/imgui.h"
#include <bx/uint32_t.h>
#include "OpenSimplex.hpp"
#include <cmath>

struct PosColor {
  float m_x;
  float m_y;
  uint32_t m_abgr;

  static void init();
  static bgfx::VertexLayout ms_layout;
};

void createCircleVB(bgfx::VertexBufferHandle* _result, float radius,
                    uint32_t color, uint16_t segments);
void createCircleIB(bgfx::IndexBufferHandle* _result, uint16_t segments);

struct dot {
    float x;
    float y;
    float s_x;
    float s_y;
    float value;
    float simplex_X;
    float simplex_Y;
    OpenSimplex* noise;

    dot() = default;
    dot(float x, float y, float s_x, float s_y, float value, OpenSimplex* noise)
        : x(x), y(y), s_x(s_x), s_y(s_y), value(value), noise(noise), simplex_X(s_x / 20), simplex_Y(s_y / 20) {}

    void update(float time) {
        double res = noise->noise3_XYBeforeZ(simplex_X, simplex_Y, time);
        res = (res + 1) / 2;
        value = float(res);
    }
};

class Wavy : public entry::AppI {
 public:
  Wavy(const char* _name, const char* _description, const char* _url);

  void init(int32_t _argc, const char* const* _argv, uint32_t _width,
            uint32_t _height) override;
  int shutdown() override;
  bool update() override;

 private:
  void updateDots(float time);
  void drawCirclesInstanced(uint64_t state);
  void drawLinesBatched(uint64_t state);
  int getOperation(int index);
  float mid(const dot* d1, const dot* d2, bool x) const;
  void getLinePos(int index, float* pos);
  void makeDots(dot*& dots, int* dot_count);
  void update_view();
  void updateDotLocations();
  float getX(int x);
  float getY(int y);

  // Edge interpolation helpers for marching squares
  float edgeUpX(int index);
  float edgeUpY(int index);
  float edgeDownX(int index);
  float edgeDownY(int index);
  float edgeLeftX(int index);
  float edgeLeftY(int index);
  float edgeRightX(int index);
  float edgeRightY(int index);

  entry::MouseState m_mouseState;
  uint32_t m_debug;
  uint32_t m_reset;
  bgfx::ProgramHandle m_program;
  bgfx::ProgramHandle m_program_inst;
  bgfx::VertexBufferHandle m_vbh;
  bgfx::IndexBufferHandle m_ibh;
  uint32_t WIDTH;
  uint32_t HEIGHT;
  uint32_t old_width;
  uint32_t old_height;
  int64_t m_timeOffset;
  OpenSimplex noise;
  dot* dots;
  const bgfx::ViewId main_view_id;
  const uint64_t state;
  const uint32_t dot_color;
  const uint16_t dot_resolution;
  float time_mult;
  int dot_count;
  int x_count;
  int y_count;
  const uint16_t grid_size;
  const float m_radius;
  const float m_eye_z;
  float fov;
  float bound;
  float y_ar;
};