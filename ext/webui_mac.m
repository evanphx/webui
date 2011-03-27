#import <Cocoa/Cocoa.h>
#import <AppKit/NSWindow.h>
#import <WebKit/WebKit.h>

#import <JavaScriptCore/JavaScriptCore.h>
#import <JavaScriptCore/JSBase.h>
#import <JavaScriptCore/JSValueRef.h>
#import <JavaScriptCore/WebKitAvailability.h>

#include "js_value.h"

struct cocoa_state;

extern void request_window_close(void* tag);

@interface WebUIController: NSWindowController <NSWindowDelegate> {

  @public
  struct cocoa_state* state;
}

- (void)windowWillClose:(NSNotification *)aNotification;
@end

struct cocoa_state {
  NSWindow* window;
  WebView* webview;
  WebUIController* controller;
  JSGlobalContextRef jsctx;
  void* tag;
};

@implementation WebUIController
- (void)windowWillClose:(NSNotification *)aNotification {
  request_window_close(state->tag);
}
@end

extern void request_prop(void* tag, const char* prop, struct js_value* val);

NSAutoreleasePool* global_pool = 0;

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
  NSString* prop = (NSString*)JSStringCopyCFString(kCFAllocatorDefault,
                                                   propertyNameJS);

  [NSMakeCollectable(prop) autorelease];

  struct js_value val;

  void* tag = JSObjectGetPrivate(object);

  request_prop(tag, [prop cStringUsingEncoding:NSASCIIStringEncoding], &val);

  return convert_to_internal(ctx, &val);
}

void plat_init() {
  [NSApplication sharedApplication];
  global_pool = [[NSAutoreleasePool alloc] init];
}

void plat_free(struct cocoa_state* state) {
  [state->window release];
  [state->controller release];
}

void* plat_create_window(void* tag, int w, int h) {
  NSRect frame = NSMakeRect(-1, -1, w, h);

  int mask = NSTitledWindowMask
           | NSClosableWindowMask
           | NSResizableWindowMask;

  struct cocoa_state* state = malloc(sizeof(struct cocoa_state));

  state->tag = tag;

  NSWindow* window  = [[NSWindow alloc] initWithContentRect:frame
                                        styleMask:mask
                                        backing:NSBackingStoreBuffered
                                        defer:NO];

  [window setBackgroundColor:[NSColor whiteColor]];
  [window makeKeyAndOrderFront:NSApp];
  [window setShowsResizeIndicator:YES];
  [window center];

  [window retain];

  state->window = window;

  state->webview = [[WebView alloc] initWithFrame:frame
                                    frameName:@"main"
                                    groupName:@"main"];

  state->controller = [WebUIController alloc];
  [state->controller retain];

  state->controller->state = state;

  [window setDelegate:state->controller];

  [window setContentView:state->webview];

  WebFrame* web_frame = [state->webview mainFrame];
  JSGlobalContextRef jsctx = [web_frame globalContext];

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

void plat_run_js(struct cocoa_state* state, char* ptr, int sz, char** ret, int* ret_sz) {
  NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];

  NSStringEncoding enc = NSASCIIStringEncoding;

  NSString* code = [[NSString alloc] initWithBytes:ptr
                                            length:sz
                                          encoding:enc];

  NSString* val = [[state->webview windowScriptObject] evaluateWebScript:code];

  int out_sz = [val lengthOfBytesUsingEncoding:enc];
  char* new_str = (char*)malloc(out_sz);
  memcpy(new_str, [val cStringUsingEncoding:enc], out_sz);

  [pool release];

  *ret = new_str;
  *ret_sz = out_sz;
}

void plat_load_url(struct cocoa_state* state, void* ptr, int sz) {
  NSStringEncoding enc = NSASCIIStringEncoding;

  NSString* s = [[NSString alloc] initWithBytes:ptr length:sz encoding:enc];

  [[state->webview mainFrame] loadRequest:
        [NSURLRequest requestWithURL:
                [NSURL URLWithString: s]]];
}

void plat_run_loop() {
  [NSApp run];
}

void plat_stop_loop() {
  [NSApp stop:NSApp];
}

void plat_run_once() {
  [[NSRunLoop currentRunLoop] runMode:NSDefaultRunLoopMode
                           beforeDate:[NSDate distantFuture]];
}

void plat_close_app() {
  if(global_pool) [global_pool release];
}
