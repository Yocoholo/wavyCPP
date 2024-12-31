#include <helpers.cpp>
#include <string>
#include <iostream>
#include <conio.h>
#include "common.h"
#include "bgfx_utils.h"
#include "imgui/imgui.h"
#include <bx/uint32_t.h>
#include <OpenSimplex.cpp>
#include <math.h>

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
void createLineVertices(PosColor*& vertices, uint16_t verticesCount,
                        float startX, float startY, float endX, float endY,
                        float m_line_thickness, uint16_t segments,
                        uint32_t color);
void createLineIndicies(uint16_t*& indicies, uint16_t& indiciesCount,
                        uint16_t verticesCount);
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

#define upX mid(&dots[index], &dots[index + 1], true);
#define upY dots[index].y;
#define downX mid(&dots[index + x_count], &dots[index + x_count +1], true);
#define downY dots[index + x_count].y;
#define leftX dots[index].x;
#define leftY mid(&dots[index], &dots[index + x_count], false);
#define rightX dots[index +1].x;
#define rightY mid(&dots[index + 1], &dots[index + x_count +1], false);

class Wavy : public entry::AppI {
 public:
  Wavy(const char* _name, const char* _description, const char* _url);

  void init(int32_t _argc, const char* const* _argv, uint32_t _width,
            uint32_t _height) override;
  int shutdown() override;
  bool update() override;

 private:
  void drawCircles(float time, uint64_t state);
  void drawCircle(dot* dot, float time, uint64_t state);
  void drawLines(uint64_t state);
  void drawLine(uint64_t state, float startX, float startY, float endX,
                float endY);
  int getOperation(int index);
  float mid(dot* d1, dot* d2, bool x);
  void getLinePos(int index, float* pos);
  void makeDots(dot*& dots, int* dot_count);
  void update_view();
  void updateDotLocations();
  float getX(int x);
  float getY(int y);

  entry::MouseState m_mouseState;
  uint32_t m_debug;
  uint32_t m_reset;
  bgfx::ProgramHandle m_program;
  bgfx::VertexBufferHandle m_vbh;
  bgfx::IndexBufferHandle m_ibh;
  uint32_t WIDTH;
  uint32_t HEIGHT;
  uint32_t old_width;
  uint32_t old_height;
  int64_t m_timeOffset;
  PosColor* all_dots_pos_color;
  OpenSimplex noise;
  dot* dots;
  bgfx::UniformHandle u_brightness;
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
  int32_t m_pt;
  bool m_r;
  bool m_g;
  bool m_b;
  bool m_a;
};