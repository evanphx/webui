#include "ruby.h"
#include "js_value.h"
#include <pthread.h>

extern void plat_init();
extern void* plat_create_window(void* tag, int w, int h);
extern void plat_free(void* state);
extern void plat_load_url(void* state, void* ptr, int size);
extern void plat_run_loop();
extern void plat_stop_loop();
extern void plat_close_app();
extern void plat_run_once();

struct window_state {
  VALUE window;
  void* plat_state;
};

static VALUE cWindow;

static VALUE init(VALUE self) {
  plat_init();
  return self;
}

static VALUE close_app(VALUE self) {
  plat_close_app();
  return self;
}

static void window_mark(void* obj) {
  struct window_state* state = obj;
  rb_gc_mark(state->window);
}

static void window_free(void* obj) {
  struct window_state* state = obj;

  plat_free(state->plat_state);
  free(obj);
}

static VALUE create_window(VALUE self, VALUE width, VALUE height) {
  struct window_state* state;
  VALUE window = Data_Make_Struct(cWindow, struct window_state, window_mark,
                                  window_free, state);

  state->window = window;
  state->plat_state = plat_create_window(state, NUM2INT(width), NUM2INT(height));

  return window;
}

#ifdef HAVE_RB_THREAD_BLOCKING_REGION

static VALUE run_loop_unlocked(void* unused) {
  plat_run_loop();
  return Qnil;
}

static VALUE run_loop(VALUE self) {
  rb_thread_blocking_region(run_loop_unlocked, 0, 0, 0);
  return self;
}

static VALUE run_once_unlocked(void* unused) {
  plat_run_once();
  return Qnil;
}

static VALUE run_once(VALUE self) {
  rb_thread_blocking_region(run_once_unlocked, 0, 0, 0);
  return self;
}

#else
static VALUE run_loop(VALUE self) {
  plat_run_loop();
  return self;
}

static VALUE run_once(VALUE self) {
  plat_run_once();
  return self;
}
#endif

static VALUE stop_loop(VALUE self) {
  plat_stop_loop();
  return self;
}

static VALUE load_url(VALUE self, VALUE url) {
  StringValue(url);

  struct window_state* state;
  Data_Get_Struct(self, struct window_state, state);

  void* ptr = RSTRING_PTR(url);
  int size = RSTRING_LEN(url);

  plat_load_url(state->plat_state, ptr, size);
  return url;
}

static VALUE run_js(VALUE self, VALUE str) {
  char* ret;
  int ret_sz;

  struct window_state* state;
  Data_Get_Struct(self, struct window_state, state);

  StringValue(str);

  plat_run_js(state->plat_state, RSTRING_PTR(str), RSTRING_LEN(str),
              &ret, &ret_sz);

  VALUE out = rb_str_new(ret, ret_sz);
  free(ret);

  return out;
}

static ID id_get_prop;
static ID id_close;

void convert_to_js_value(VALUE obj, struct js_value* val) {
  if(obj == Qnil) {
    val->type = js_value_null;
  } else if(obj == Qtrue || obj == Qfalse) {
    val->type = js_value_bool;
    val->v.integer = (obj == Qtrue);
  } else if(TYPE(obj) == T_STRING) {
    StringValue(obj);

    val->type = js_value_string;
    val->v.ptr = RSTRING_PTR(obj);
  } else if(FIXNUM_P(obj)) {
    val->type = js_value_number;
    val->v.number = (double)NUM2INT(obj);
  } else {
    val->type = js_value_null;
  }
}

#ifdef HAVE_RB_THREAD_BLOCKING_REGION

struct prop_request {
  void* tag;
  const char* prop;
  struct js_value* val;
};

void* request_prop_unlocked(void* arg) {
  struct prop_request* req = arg;
  struct window_state* state = req->tag;

  VALUE ret = rb_funcall(state->window, id_get_prop, 1, rb_str_new2(req->prop));
  convert_to_js_value(ret, req->val);
  return 0;
}

void request_prop(void* tag, const char* prop, struct js_value* val) {
  struct prop_request req = {tag, prop, val};
  rb_thread_call_with_gvl(request_prop_unlocked, &req);
}
#else
void request_prop(void* tag, const char* prop, struct js_value* val) {
  struct window_state* state = tag;

  VALUE ret = rb_funcall(state->window, id_get_prop, 1, rb_str_new2(prop));
  convert_to_js_value(ret, val);
}
#endif

#ifdef HAVE_RB_THREAD_BLOCKING_REGION

void* request_window_close_locked(void* tag) {
  struct window_state* state = tag;
  rb_funcall(state->window, id_close, 0);
  return 0;
}

void request_window_close(void* tag) {
  rb_thread_call_with_gvl(&request_window_close_locked, tag);
}

#else
void request_window_close(void* tag) {
  struct window_state* state = tag;
  rb_funcall(state->window, id_close, 0);
}
#endif

void Init_webui_platform() {
  id_get_prop = rb_intern("get_property");
  id_close = rb_intern("close");

  VALUE cls = rb_define_class("WebUI", rb_cObject);
  rb_define_singleton_method(cls, "init", init, 0);
  rb_define_singleton_method(cls, "create_window", create_window, 2);
  rb_define_singleton_method(cls, "run", run_loop, 0);
  rb_define_singleton_method(cls, "stop", stop_loop, 0);
  rb_define_singleton_method(cls, "run_once", run_once, 0);
  rb_define_singleton_method(cWindow, "close", close_app, 0);

  cWindow = rb_define_class_under(cls, "Window", rb_cObject);
  rb_global_variable(&cWindow);

  rb_define_method(cWindow, "load_url", load_url, 1);
  rb_define_method(cWindow, "run_js", run_js, 1);
}
