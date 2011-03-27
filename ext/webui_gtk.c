#include <string.h>
#include <stdlib.h>

#include <gtk/gtk.h>
#include <webkit/webkit.h>

#include <JavaScriptCore/JavaScript.h>
#include <JavaScriptCore/JSBase.h>
#include <JavaScriptCore/JSValueRef.h>
#include <JavaScriptCore/WebKitAvailability.h>

#include "js_value.h"

struct gtk_state {
  GtkWidget* window;
  GtkWidget* web_view;
  JSGlobalContextRef jsctx;
  void* tag;
};

void plat_init() {
  int argc = 0;
  char** argv = 0;

  gtk_init(&argc, &argv);
}

static void close_window(GtkWidget* widget, gpointer data) {
  struct gtk_state* state = data;
  request_window_close(state->tag);
}

extern void request_prop(void* tag, const char* prop, struct js_value* val);

JSValueRef convert_to_internal(JSContextRef ctx, struct js_value* val) {
  switch(val->type) {
  default:
  case js_value_null:
    return JSValueMakeNull(ctx);
    break;
  case js_value_bool:
    return JSValueMakeBoolean(ctx, val->v.integer);
    break;
  case js_value_string:
    return JSValueMakeString(ctx,
             JSStringCreateWithUTF8CString(val->v.ptr));
    break;
  case js_value_number:
    return JSValueMakeNumber(ctx, val->v.number);
    break;
  }
};

JSValueRef ruby_getprop(JSContextRef ctx, JSObjectRef object,
                        JSStringRef propertyNameJS,
                        JSValueRef* exception)
{
  int out_sz = JSStringGetLength(propertyNameJS);
  char* new_str = malloc(out_sz + 1);

  const JSChar* chars = JSStringGetCharactersPtr(propertyNameJS);
  int i;
  for(i = 0; i < out_sz; i++) {
    new_str[i] = (char)chars[i];
  }

  new_str[out_sz] = 0;

  struct js_value val;

  void* tag = JSObjectGetPrivate(object);

  request_prop(tag, new_str, &val);

  free(new_str);

  return convert_to_internal(ctx, &val);
}

void* plat_create_window(void* tag, int w, int h) {
  struct gtk_state* state = malloc(sizeof(struct gtk_state));
  state->tag = tag;

  GtkWidget* window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(window), "WebUI");

  state->window = window;

  gtk_signal_connect(GTK_OBJECT(window), "destroy",
                     GTK_SIGNAL_FUNC(close_window), state);

  GtkWidget* scroll = gtk_scrolled_window_new(NULL, NULL);
  state->web_view = webkit_web_view_new();

  gtk_container_add(GTK_CONTAINER(scroll), state->web_view);
  gtk_container_add(GTK_CONTAINER(window), scroll);

  WebKitWebFrame* web_frame = webkit_web_view_get_main_frame(
                              WEBKIT_WEB_VIEW(state->web_view));

  gtk_window_set_default_size(GTK_WINDOW(window), w, h);
  gtk_widget_show_all(window);

  JSGlobalContextRef jsctx = webkit_web_frame_get_global_context(web_frame);

  state->jsctx = jsctx;

  JSClassDefinition system_def = kJSClassDefinitionEmpty;
  system_def.className = "ruby";
  system_def.getProperty = ruby_getprop;

  JSClassRef system_class = JSClassCreate(&system_def);

  JSObjectRef o = JSObjectMake(jsctx, system_class, NULL);
  if(!JSObjectSetPrivate(o, tag)) {
    printf("WebKit is busted.\n");
    abort();
  }

  JSStringRef	name = JSStringCreateWithUTF8CString("ruby");
  JSObjectSetProperty(jsctx, JSContextGetGlobalObject(jsctx), name, o,
                      kJSPropertyAttributeDontDelete, NULL);
  JSStringRelease(name);

  return state;
}

void plat_run_js(struct gtk_state* state, char* ptr, int sz, char** ret, int* ret_sz) {
  JSValueRef error;
  JSStringRef body = JSStringCreateWithUTF8CString(ptr);
  JSObjectRef obj = JSObjectMakeFunction(state->jsctx, NULL, 0, 0, body,
                                         0, 0, &error);

  JSValueRef val = JSObjectCallAsFunction(state->jsctx, obj, NULL, 0, 0, &error);
  JSStringRef str = JSValueToStringCopy(state->jsctx, val, &error);

  int out_sz = JSStringGetLength(str);
  char* new_str = malloc(out_sz);

  const JSChar* chars = JSStringGetCharactersPtr(str);
  int i;
  for(i = 0; i < out_sz; i++) {
    new_str[i] = (char)chars[i];
  }

  JSStringRelease(str);

  *ret = new_str;
  *ret_sz = out_sz;
}

void plat_load_url(struct gtk_state* state, void* ptr, int sz) {
  webkit_web_view_open(WEBKIT_WEB_VIEW(state->web_view), (const gchar*)ptr);
}

void plat_run_loop() {
  gtk_main();
}

void plat_stop_loop() {
  gtk_main_quit();
}

void plat_run_once() {
  gtk_main_iteration();
}

void plat_close_app() {
  // nothing, no cleanup.
}

void plat_free() {
  // gtk cleanup?
}
