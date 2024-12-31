#include "wavy.hpp"

bgfx::VertexLayout PosColor::ms_layout;

void PosColor::init() {
  ms_layout.begin()
      .add(bgfx::Attrib::Position, 2, bgfx::AttribType::Float)
      .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
      .end();
}

void createCircleVB(bgfx::VertexBufferHandle* _result, float radius,
                    uint32_t color, uint16_t segments) {
  uint16_t numVertices = segments + 1;
  uint32_t size = numVertices * sizeof(PosColor);
  PosColor* vertex = new PosColor[numVertices];
  vertex[0] = PosColor{0.0f, 0.0f, color};
  float angleStep = 2.0f * M_PI / segments;
  float angle = 0.0f;
  for (uint16_t i = 1; i < segments + 1; i++) {
    vertex[i] = PosColor{radius * cosf(angle), radius * sinf(angle), color};
    angle += angleStep;
  }
  *_result =
      bgfx::createVertexBuffer(bgfx::copy(vertex, size), PosColor::ms_layout);
}

void createCircleIB(bgfx::IndexBufferHandle* _result, uint16_t segments) {
  uint16_t numIndices = segments * 3;
  uint32_t size = numIndices * sizeof(uint16_t);
  uint16_t* index = new uint16_t[numIndices];
  int i_offset = 0;
  for (uint16_t i = 0; i < segments; i++) {
    index[i_offset] = 0;
    index[i_offset + 1] = i + 1;
    index[i_offset + 2] = i + 2;
    i_offset += 3;
  }
  index[numIndices - 1] = 1;
  *_result = bgfx::createIndexBuffer(bgfx::copy(index, size));
}

void createLineVertices(PosColor*& vertices, uint16_t verticesCount,
                        float startX, float startY, float endX, float endY,
                        float m_line_thickness, uint16_t segments,
                        uint32_t color) {
  float angle = atan2f(endY - startY, endX - startX);
  float angle_step = M_PI / segments;
  float start_angle = angle + M_PI_2;
  float end_angle = angle - M_PI_2;
  for (int i = 0; i < segments * 2; i += 2) {
    vertices[i] =
        PosColor{startX + m_line_thickness * cosf(start_angle),
                 startY + m_line_thickness * sinf(start_angle), color};
    vertices[i + 1] =
        PosColor{endX + m_line_thickness * cosf(end_angle),
                 endY + m_line_thickness * sinf(end_angle), color};
    start_angle += angle_step;
    end_angle += angle_step;
  }
}

void createLineIndicies(uint16_t*& indicies, uint16_t& indiciesCount,
                        uint16_t verticesCount) {
  indiciesCount = verticesCount + 1;
  int oddIndex = 0;
  int evenIndex = (verticesCount) / 2;
  for (int i = 0; i < verticesCount; ++i) {
    if (i % 2 != 0) {
      indicies[oddIndex] = i;
      oddIndex++;
    } else {
      indicies[evenIndex] = i;
      evenIndex++;
    }
  }
  indicies[verticesCount] = 1;
}

Wavy::Wavy(const char* _name, const char* _description, const char* _url)
    : entry::AppI(_name, _description, _url),
      m_pt(0),
      m_r(true),
      m_g(true),
      m_b(true),
      m_a(true),
      noise(1738),
      main_view_id(0),
      state(0 | BGFX_STATE_WRITE_R | BGFX_STATE_WRITE_G | BGFX_STATE_WRITE_B |
            BGFX_STATE_WRITE_A | BGFX_STATE_WRITE_Z |
            BGFX_STATE_DEPTH_TEST_LESS | BGFX_STATE_CULL_CW | BGFX_STATE_MSAA),
      dot_color(0xffffffff),
      dot_resolution(10),
      time_mult(0.4f),
      grid_size(267),
      m_radius(0.1f),
      m_eye_z(15.0f),
      fov(120),
      bound(0.0f),
      y_ar(0.0f) {}

void Wavy::init(int32_t _argc, const char* const* _argv, uint32_t _width,
                uint32_t _height) {
  Args args(_argc, _argv);
  WIDTH = _width;
  HEIGHT = _height;
  old_width = 0;
  old_height = 0;
  m_debug = BGFX_DEBUG_TEXT;
  m_reset = BGFX_RESET_VSYNC;
  std::cout << "width: " << _width << " height: " << _height << std::endl;
  y_ar = m_eye_z * float(HEIGHT) / float(WIDTH);
  bgfx::Init init;
  init.type = args.m_type;
  init.vendorId = args.m_pciId;
  init.platformData.nwh =
      entry::getNativeWindowHandle(entry::kDefaultWindowHandle);
  init.platformData.ndt = entry::getNativeDisplayHandle();
  init.platformData.type = entry::getNativeWindowHandleType();
  init.resolution.width = WIDTH;
  init.resolution.height = HEIGHT;
  init.resolution.reset = m_reset;
  if (!bgfx::init(init)) {
    return;
  }
  bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x0, 1.0f, 0);
  PosColor::init();
  makeDots(dots, &dot_count);
  createCircleVB(&m_vbh, m_radius, dot_color, dot_resolution);
  createCircleIB(&m_ibh, dot_resolution);
  u_brightness = bgfx::createUniform("u_brightness", bgfx::UniformType::Vec4);
  m_program = loadProgram("vs_wavy", "fs_wavy");
  m_timeOffset = bx::getHPCounter();
  imguiCreate();
}

int Wavy::shutdown() {
  imguiDestroy();
  bgfx::destroy(u_brightness);
  bgfx::destroy(m_ibh);
  bgfx::destroy(m_vbh);
  bgfx::destroy(m_program);
  bgfx::shutdown();
  return 0;
}

bool Wavy::update() {
  if (!entry::processEvents(WIDTH, HEIGHT, m_debug, m_reset, &m_mouseState)) {
    imguiBeginFrame(
        m_mouseState.m_mx, m_mouseState.m_my,
        (m_mouseState.m_buttons[entry::MouseButton::Left] ? IMGUI_MBUT_LEFT
                                                          : 0) |
            (m_mouseState.m_buttons[entry::MouseButton::Right]
                 ? IMGUI_MBUT_RIGHT
                 : 0) |
            (m_mouseState.m_buttons[entry::MouseButton::Middle]
                 ? IMGUI_MBUT_MIDDLE
                 : 0),
        m_mouseState.m_mz, uint16_t(WIDTH), uint16_t(HEIGHT));
    imguiEndFrame();

    if (old_width != WIDTH || old_height != HEIGHT) update_view();

    bgfx::touch(0);
    float time = (float)((bx::getHPCounter() - m_timeOffset) /
                         double(bx::getHPFrequency())) *
                 time_mult;

    drawCircles(time, state);
    drawLines(state);
    bgfx::frame();
    return true;
  }
  return false;
}

void Wavy::drawCircles(float time, uint64_t state) {
  state |= UINT64_C(0);
  for (int i = 0; i < dot_count; i++) {
    drawCircle(&dots[i], time, state);
  }
}

void Wavy::drawCircle(dot* dot, float time, uint64_t state) {
  dot->update(time);
  float brightness[16] = {dot->value, 0.0f, 0.0f, 0.0f};
  bgfx::setUniform(u_brightness, brightness);
  float mtx[16];
  bx::mtxTranslate(mtx, dot->x, dot->y, 0.0f);
  bgfx::setTransform(mtx);
  bgfx::setVertexBuffer(0, m_vbh);
  bgfx::setIndexBuffer(m_ibh);
  bgfx::setState(state);
  bgfx::submit(main_view_id, m_program);
}

void Wavy::drawLines(uint64_t state) {
  state |= BGFX_STATE_PT_LINES;
  for (int i = 0; i < dot_count; i++) {
    if (dots[i].s_x == x_count - 1 || dots[i].s_y == y_count - 1) continue;
    float pos[8];
    getLinePos(i, pos);
    if (pos[0] == -1) continue;
    drawLine(state, pos[0], pos[1], pos[2], pos[3]);
    if (pos[4] == -1) continue;
    drawLine(state, pos[4], pos[5], pos[6], pos[7]);
  }
}

void Wavy::drawLine(uint64_t state, float startX, float startY, float endX,
                    float endY) {
  bgfx::TransientVertexBuffer tvb;
  bgfx::TransientIndexBuffer tib;

  PosColor* vertices;
  uint16_t* indicies;
  uint16_t verticesCount = 2;
  uint16_t indiciesCount = 2;
  bgfx::allocTransientBuffers(&tvb, PosColor::ms_layout, verticesCount, &tib,
                              indiciesCount);
  vertices = (PosColor*)tvb.data;
  indicies = (uint16_t*)tib.data;

  vertices[0].m_x = startX;
  vertices[0].m_y = startY;
  vertices[0].m_abgr = dot_color;
  vertices[1].m_x = endX;
  vertices[1].m_y = endY;
  vertices[1].m_abgr = dot_color;

  indicies[0] = 0;
  indicies[1] = 1;

  float brightness[16] = {1.0f, 0.0f, 0.0f, 0.0f};
  bgfx::setUniform(u_brightness, brightness);

  bgfx::setVertexBuffer(0, &tvb);
  bgfx::setIndexBuffer(&tib);
  bgfx::setState(state);
  bgfx::submit(main_view_id, m_program);
}

int Wavy::getOperation(int index) {
  return (dots[index].value >= 0.5f ? 1 : 0) +
         (dots[index + 1].value >= 0.5f ? 2 : 0) +
         (dots[index + x_count + 1].value >= 0.5f ? 4 : 0) +
         (dots[index + x_count].value >= 0.5f ? 8 : 0);
}

float Wavy::mid(dot* d1, dot* d2, bool x) {
  float v1 = x ? d1->x : d1->y;
  float v2 = x ? d2->x : d2->y;
  return (v1 * d1->value + v2 * d2->value) / (d1->value + d2->value);
}

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
      pos[0] = upX;
      pos[1] = upY;
      pos[2] = leftX;
      pos[3] = leftY;
      break;
    case 2:
    case 13:
      pos[0] = upX;
      pos[1] = upY;
      pos[2] = rightX;
      pos[3] = rightY;
      break;
    case 4:
    case 11:
      pos[0] = downX;
      pos[1] = downY;
      pos[2] = rightX;
      pos[3] = rightY;
      break;
    case 8:
    case 7:
      pos[0] = downX;
      pos[1] = downY;
      pos[2] = leftX;
      pos[3] = leftY;
      break;
    case 3:
    case 12:
      pos[0] = leftX;
      pos[1] = leftY;
      pos[2] = rightX;
      pos[3] = rightY;
      break;
    case 6:
    case 9:
      pos[0] = upX;
      pos[1] = upY;
      pos[2] = downX;
      pos[3] = downY;
      break;
    case 5:
      pos[0] = upX;
      pos[1] = upY;
      pos[2] = rightX;
      pos[3] = rightY;
      pos[4] = downX;
      pos[5] = downY;
      pos[6] = leftX;
      pos[7] = leftY;
      break;
    case 10:
      pos[0] = upX;
      pos[1] = upY;
      pos[2] = leftX;
      pos[3] = leftY;
      pos[4] = downX;
      pos[5] = downY;
      pos[6] = rightX;
      pos[7] = rightY;
      break;
    default:
      break;
  }
}

void Wavy::makeDots(dot*& dots, int* dot_count) {
  x_count = grid_size;
  y_count = grid_size * HEIGHT / WIDTH;
  *dot_count = x_count * y_count;
  dots = new dot[*dot_count];
  for (int y = 0; y < y_count; y++) {
    for (int x = 0; x < x_count; x++) {
      int i = y * x_count + x;
      dots[i] = dot(getX(x), getY(y), float(x), float(y), 0.0f, &noise);
    }
  }
}

void Wavy::update_view() {
  old_width = WIDTH;
  old_height = HEIGHT;
  const bx::Vec3 at = {0.0f, 0.0f, 0.0f};
  const bx::Vec3 eye = {0.0f, 0.0f, -m_eye_z};
  float proj[16];
  float view[16];
  float inv_proj[16];
  float world_pos[4] = {1.0f, 1.0f, 0.0f, 0.0f};
  bx::mtxLookAt(view, eye, at);
  bx::mtxProj(proj, fov, float(WIDTH) / float(HEIGHT), 0.1f, 100.0f,
              bgfx::getCaps()->homogeneousDepth);
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
      int i = y * x_count + x;
      dots[i].x = getX(x);
      dots[i].y = getY(y);
    }
  }
}

float Wavy::getX(int x) {
  return (bound * m_eye_z *
          (2 * (float(x) * (float(WIDTH) / float(x_count - 1))) - WIDTH)) /
         WIDTH;
}

float Wavy::getY(int y) {
  return (bound * y_ar *
          (HEIGHT - 2 * (float(y) * (float(HEIGHT) / float(y_count - 1))))) /
         HEIGHT;
}


ENTRY_IMPLEMENT_MAIN(
    Wavy, "Wavy", "My lil Wavy app", "https://google.com");

int _main_(int _argc, char **_argv) { return 0; };