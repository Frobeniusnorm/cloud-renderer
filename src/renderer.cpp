#include "renderer.hpp"
#include "shader.hpp"
#include "vao.hpp"
#include <GL/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <gtkmm.h>
static const float vertices[24]{
    -1, 0, 2,  // 0
    1,  0, 2,  // 1
    -1, 1, 2,  // 2
    1,  1, 2,  // 3
    -1, 0, -1, // 4
    1,  0, -1, // 5
    -1, 1, -1, // 6
    1,  1, -1  // 7
};
static const unsigned int indices[36]{// Top
                                      2, 6, 7, 2, 3, 7,

                                      // Bottom
                                      0, 4, 5, 0, 1, 5,

                                      // Left
                                      0, 2, 6, 0, 4, 6,

                                      // Right
                                      1, 3, 7, 1, 5, 7,

                                      // Front
                                      0, 2, 3, 0, 1, 3,

                                      // Back
                                      4, 6, 7, 4, 5, 7};
static glm::mat4 last_mat;
static int last_width, last_height;
static float angle_r = 1.0472, angle_p = 0;
static void update_camera_matrix(int width, int height) {
  last_width = width;
  last_height = height;
  using namespace glm;
  static const float radius = 3.f;
  vec3 eye = vec3(0, 0.5, 0.5) + radius * vec3(sin(angle_r) * sin(angle_p),
                                               cos(angle_r),
                                               sin(angle_r) * cos(angle_p));
  mat4 view = lookAt(eye, vec3(0, 0, 0.5f), vec3(0, 1, 0));
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
}
void cloud_renderer::set_view_angle_y(float p) {
  angle_r = (p / 360.0) * 2 * 3.141592;
  update_camera_matrix(last_width, last_height);
}
void cloud_renderer::set_view_angle_x(float r) {
  angle_p = (r / 360.0) * 2 * 3.141592;
  update_camera_matrix(last_width, last_height);
}
void cloud_renderer::resize(int width, int height) {
  update_camera_matrix(width, height);
}
void cloud_renderer::cleanup() {
  delete render_box;
  delete program;
}
bool cloud_renderer::render(const Glib::RefPtr<Gdk::GLContext> &context) {
  if (!render_box) {
    init();
  }
  glClearColor(0.2, 0.2, 0.5, 0);
  glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
  glEnable(GL_DEPTH_TEST);
  program->start();
  program->load("cammat", last_mat);
  render_box->bind();
  render_box->draw();
  render_box->unbind();
  program->stop();
  return true;
}
