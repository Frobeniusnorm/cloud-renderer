#ifndef RENDERER_HPP
#define RENDERER_HPP
#include <gtkmm.h>
namespace cloud_renderer {
void init();
void cleanup();
bool render(const Glib::RefPtr<Gdk::GLContext> &context);
void resize(int width, int height);
void set_view_angle_x(float r);
void set_view_angle_y(float p);
void set_step_size(float ss);
} // namespace cloud_renderer
#endif
