#include "gtkmm/glarea.h"
#include "gtkmm/scrolledwindow.h"
#include "renderer.hpp"
#include "sigc++/functors/mem_fun.h"
#include "sigc++/functors/ptr_fun.h"
#include <gtkmm.h>
static bool signal_x_rotation(Gtk::ScrollType, double newval) {
  cloud_renderer::set_view_angle_x(newval);
  return true;
}
static bool signal_y_rotation(Gtk::ScrollType, double newval) {
  cloud_renderer::set_view_angle_y(newval);
  return true;
}
class CloudWindow : public Gtk::Window {
  Gtk::Paned divider;
  Gtk::Notebook settings_notebook;
  Gtk::Label l1, l2, l3;
  Gtk::GLArea cloud_window;
  bool on_tick(const Glib::RefPtr<Gdk::FrameClock> &frame_clock) {
    cloud_window.queue_render();
    return true;
  }
  // render settings
  Gtk::Scale x_rotation, y_rotation;
  Gtk::ScrolledWindow render_settings;
  Gtk::Box render_settings_content, x_rotation_box, y_rotation_box;
  void construct_render_settings() {
    render_settings.set_child(render_settings_content);
    Gtk::Scale *scales[2] = {&x_rotation, &y_rotation};
    Gtk::Box *boxes[2] = {&x_rotation_box, &y_rotation_box};
    for (int i = 0; i < 2; i++) {
      scales[i]->set_range(0, 360);
      scales[i]->set_hexpand();
      Gtk::Label label((i == 0 ? "x" : "y") + std::string("-rotation:"));
      boxes[i]->append(label);
      boxes[i]->set_margin_start(15);
      boxes[i]->set_hexpand();
      boxes[i]->append(*scales[i]);
      render_settings_content.append(*boxes[i]);
    }
    x_rotation.set_value(0);
    y_rotation.set_value(60);
    x_rotation.signal_change_value().connect(sigc::ptr_fun(&signal_x_rotation),
                                             true);
    y_rotation.signal_change_value().connect(sigc::ptr_fun(&signal_y_rotation),
                                             true);
  }

public:
  CloudWindow()
      : divider(Gtk::Orientation::HORIZONTAL), settings_notebook(),
        l1("Clouds"), l2("Render"), l3("Clouds Renderer"), cloud_window(),
        x_rotation(Gtk::Orientation::HORIZONTAL),
        y_rotation(Gtk::Orientation::HORIZONTAL),
        render_settings_content(Gtk::Orientation::VERTICAL),
        x_rotation_box(Gtk::Orientation::HORIZONTAL),
        y_rotation_box(Gtk::Orientation::HORIZONTAL) {
    set_title("Cloud simulation");
    maximize();
    set_child(divider);
    set_default_size(850, 600);
    divider.set_margin(10);
    settings_notebook.append_page(l1, "Clouds");
    settings_notebook.append_page(render_settings, "Renderer");
    settings_notebook.set_margin(0);
    settings_notebook.set_size_request(300, -1);
    settings_notebook.set_show_border(false);
    cloud_window.set_has_depth_buffer(true);
    cloud_window.signal_render().connect(sigc::ptr_fun(&cloud_renderer::render),
                                         true);
    cloud_window.signal_resize().connect(sigc::ptr_fun(&cloud_renderer::resize),
                                         true);
    cloud_window.set_auto_render();
    cloud_window.add_tick_callback(sigc::mem_fun(*this, &CloudWindow::on_tick));
    divider.set_margin(0);
    divider.set_resize_end_child(false);
    divider.set_end_child(settings_notebook);
    divider.set_start_child(cloud_window);
    construct_render_settings();
  }
  ~CloudWindow() { cloud_renderer::cleanup(); }
};

int main(int argc, char *argv[]) {
  auto app = Gtk::Application::create("");

  return app->make_window_and_run<CloudWindow>(argc, argv);
}
