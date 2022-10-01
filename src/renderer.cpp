#include "renderer.hpp"
#include "framebuffer.hpp"
#include "shader.hpp"
#include "simplex.hpp"
#include "texture.hpp"
#include "vao.hpp"
#include <GL/gl.h>
#include <chrono>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <gtkmm.h>
#include <random>
static const float vertices[24]{-1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f,
                                1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f,
                                -1.0f, -1.0f, 2.0f,  1.0f,  -1.0f, 2.0f,
                                1.0f,  1.0f,  2.0f,  -1.0f, 1.0f,  2.0f};
static const unsigned int indices[36]{// Top
                                      5, 4, 0, 1, 5, 0, 6, 5, 1, 2, 6, 1,
                                      7, 6, 2, 3, 7, 2, 4, 7, 3, 0, 4, 3,
                                      6, 7, 4, 5, 6, 4, 1, 0, 3, 2, 1, 3};
static void GLAPIENTRY messageCallback(GLenum source, GLenum type, GLuint id,
                                       GLenum severity, GLsizei length,
                                       const GLchar *message,
                                       const void *userParam) {
  std::cerr << std::string(message) << std::endl;
}
static GLuint noise2D;
static glm::mat4 last_mat;
static glm::vec3 last_eye;
static int last_width, last_height;
static float angle_r = 1.0472, angle_p = 0, stepSize = 0.02, radius_scale = 1.0;
static Framebuffer *back_side = nullptr;
static std::chrono::steady_clock::time_point start_point;
static void update_camera_matrix(int width, int height) {
  last_width = width;
  last_height = height;
  using namespace glm;
  float radius = 3.f * radius_scale;
  last_eye = vec3(0, 0.5, 0.5) + radius * vec3(sin(angle_r) * sin(angle_p),
                                               cos(angle_r),
                                               sin(angle_r) * cos(angle_p));
  mat4 view = lookAt(last_eye, vec3(0, 0, 0.5f), vec3(0, 1, 0));
  mat4 proj =
      perspective(radians(50.f), (float)width / (float)height, 0.1f, 10.0f);
  last_mat = proj * view;
}
static Vao *render_box = nullptr;
static ShaderProgram *program = nullptr;
;
void cloud_renderer::init() {
  if (glewInit() != GLEW_OK) {
    std::cerr << "GLEW not initialized!" << std::endl;
  }

  program = new ShaderProgram("shader/cloudbox_vert.glsl",
                              "shader/cloudbox_frag.glsl", {"coords"});
  render_box = new Vao();
  render_box->addVertexBuffer(3, &vertices[0], 72);
  render_box->addIndexBuffer(&indices[0], 36);
  glEnable(GL_DEBUG_OUTPUT);
  glDebugMessageCallback(messageCallback, 0);
  glEnable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);
  // Generate Noise
  std::vector<float> nd2d(256 * 256 * 2);
#pragma omp parallel for
  for (int i = 0; i < 256; i++)
    for (int j = 0; j < 256; j++) {

      nd2d[i * 256 * 2 + j * 2] =
          SimplexNoise::noise(i / 64.0, j / 64.0, (i + j) / 64.0);
      nd2d[i * 256 * 2 + j * 2 + 1] =
          SimplexNoise::noise(i / 64.0, j / 64.0, (j - i) / 64.0);
    }
  noise2D = Texture::loadBinary(nd2d.data(), 256, 256, 2);
  start_point = std::chrono::steady_clock::now();
}
void cloud_renderer::set_view_angle_y(float p) {
  angle_r = (p / 360.0) * 2 * 3.141592;
  update_camera_matrix(last_width, last_height);
}
void cloud_renderer::set_view_angle_x(float r) {
  angle_p = (r / 360.0) * 2 * 3.141592;
  update_camera_matrix(last_width, last_height);
}
void cloud_renderer::set_radius(float r) {
  radius_scale = r;
  update_camera_matrix(last_width, last_height);
}
void cloud_renderer::set_step_size(float ss) { stepSize = ss; }
void cloud_renderer::resize(int width, int height) {
  if (!back_side && program) {
    back_side = new Framebuffer(width, height);
    back_side->generateColorTexture(GL_RGBA32F);
    back_side->generateDepthBuffer();
  } else if (program)
    back_side->resize(width, height);
  update_camera_matrix(width, height);
}
void cloud_renderer::cleanup() {
  glDeleteTextures(1, &noise2D);
  delete render_box;
  delete program;
  if (back_side)
    delete back_side;
}
bool cloud_renderer::render(const Glib::RefPtr<Gdk::GLContext> &context) {
  if (!render_box) {
    init();
  }
  glClearColor(0.2, 0.2, 0.5, 1.0);
  glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
  if (!back_side)
    resize(last_width, last_height);
  program->start();
  render_box->bind();
  program->load("cammat", last_mat);
  program->load("eye", last_eye);

  // backside
  back_side->bind();
  glClearColor(0, 0, 0, 0);
  glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
  glCullFace(GL_BACK);
  program->load("backside", 0);
  render_box->draw();
  back_side->unbind();

  // front side
  glCullFace(GL_FRONT);
  program->load("backside", 1);
  program->load("stepSize", stepSize);
  program->load(
      "time", ((std::chrono::duration<float>)(std::chrono::steady_clock::now() -
                                              start_point))
                  .count());
  program->loadTexture("frontside_tex", back_side->getColorTexture(), 0);
  program->loadTexture("noise2D", noise2D, 1);
  render_box->draw();
  render_box->unbind();
  program->stop();
  return true;
}
