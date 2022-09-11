#include "renderer.hpp"
#include "framebuffer.hpp"
#include "shader.hpp"
#include "vao.hpp"
#include <GL/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <gtkmm.h>
static const float vertices[24]{-1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f,
                                1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f,
                                -1.0f, -1.0f, 2.0f,  1.0f,  -1.0f, 2.0f,
                                1.0f,  1.0f,  2.0f,  -1.0f, 1.0f,  2.0f};
static const unsigned int indices[36]{// Top
                                      5, 4, 0, 1, 5, 0, 6, 5, 1, 2, 6, 1,
                                      7, 6, 2, 3, 7, 2, 4, 7, 3, 0, 4, 3,
                                      6, 7, 4, 5, 6, 4, 1, 0, 3, 2, 1, 3};
static glm::mat4 last_mat;
static glm::vec3 last_eye;
static int last_width, last_height;
static float angle_r = 1.0472, angle_p = 0;
static Framebuffer *back_side = nullptr;
static void update_camera_matrix(int width, int height) {
  last_width = width;
  last_height = height;
  using namespace glm;
  static const float radius = 3.f;
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
  glEnable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);
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
  if (!back_side && program) {
    back_side = new Framebuffer(width, height);
    back_side->generateColorTexture();
    back_side->generateDepthBuffer();
  } else if (program)
    back_side->resize(width, height);
  update_camera_matrix(width, height);
}
void cloud_renderer::cleanup() {
  delete render_box;
  delete program;
  if (back_side)
    delete back_side;
}
bool cloud_renderer::render(const Glib::RefPtr<Gdk::GLContext> &context) {
  if (!render_box) {
    init();
  }
  glClearColor(0.2, 0.2, 0.5, 0);
  glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
  if (!back_side)
    resize(context->get_surface()->get_width(),
           context->get_surface()->get_height());
  program->start();
  render_box->bind();
  program->load("cammat", last_mat);
  program->load("eye", last_eye);

  // backside
  back_side->bind();
  glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
  glCullFace(GL_FRONT);
  program->load("backside", true);
  render_box->draw();
  back_side->unbind();

  // front side
  glCullFace(GL_BACK);
  program->load("backside", false);
  program->loadTexture("backside_tex", back_side->getColorTexture(), 0);
  render_box->draw();
  render_box->unbind();
  program->stop();
  return true;
}
