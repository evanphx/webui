#include "ruby.h"

extern void Init_webui_platform_mac(VALUE);
extern void plat_create_window(int w, int h);
extern void plat_load_url(void* ptr, int size);
extern void plat_run_window();

VALUE create_window(VALUE self, VALUE width, VALUE height) {
  plat_create_window(NUM2INT(width), NUM2INT(height));
  return self;
}

VALUE load_url(VALUE self, VALUE url) {
  StringValue(url);

  void* ptr = RSTRING_PTR(url);
  int size = RSTRING_LEN(url);

  plat_load_url(ptr, size);
}

VALUE run_window(VALUE self) {
  plat_run_window();
  return self;
}

void Init_webui_platform() {
  VALUE cls = rb_define_class("WebUI", rb_cObject);
  rb_define_singleton_method(cls, "create_window", create_window, 2);
  rb_define_singleton_method(cls, "load_url", load_url, 1);
  rb_define_singleton_method(cls, "run", run_window, 0);
}
