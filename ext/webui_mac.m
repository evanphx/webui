#import <Cocoa/Cocoa.h>
#import <AppKit/NSWindow.h>
#import <WebKit/WebKit.h>

@interface Silly : NSWindowController <NSWindowDelegate>
- (void)windowWillClose:(NSNotification *)aNotification;
@end

@implementation Silly
- (void)windowWillClose:(NSNotification *)aNotification {
  [NSApp stop:self];
}
@end

NSAutoreleasePool* global_pool = 0;
WebView* global_webview = 0;

void plat_create_window(int w, int h) {
  [NSApplication sharedApplication];

  global_pool = [[NSAutoreleasePool alloc] init];

  NSRect frame = NSMakeRect(-1, -1, w, h);

  int mask = NSTitledWindowMask
           | NSClosableWindowMask
           | NSResizableWindowMask;

  NSWindow* window  = [[NSWindow alloc] initWithContentRect:frame
                                        styleMask:mask
                                        backing:NSBackingStoreBuffered
                                        defer:NO];

  [window setBackgroundColor:[NSColor whiteColor]];
  [window makeKeyAndOrderFront:NSApp];
  [window setShowsResizeIndicator:YES];

  global_webview = [[WebView alloc] initWithFrame:frame
                                    frameName:@"main"
                                    groupName:@"main"];

  Silly* silly = [Silly alloc];
  [window setDelegate:silly];

  [window setContentView:global_webview];
}

void plat_run_js(char* ptr, int sz, char** ret, int* ret_sz) {
  NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];

  NSStringEncoding enc = NSASCIIStringEncoding;

  NSString* code = [[NSString alloc] initWithBytes:ptr
                                            length:sz
                                          encoding:enc];

  NSString* val = [[global_webview windowScriptObject] evaluateWebScript:code];

  int out_sz = [val lengthOfBytesUsingEncoding:enc];
  char* new_str = (char*)malloc(out_sz);
  memcpy(new_str, [val cStringUsingEncoding:enc], out_sz);

  [pool release];

  *ret = new_str;
  *ret_sz = out_sz;
}

void plat_load_url(void* ptr, int sz) {
  NSStringEncoding enc = NSASCIIStringEncoding;

  NSString* s = [[NSString alloc] initWithBytes:ptr length:sz encoding:enc];

  [[global_webview mainFrame] loadRequest:
        [NSURLRequest requestWithURL:
                [NSURL URLWithString: s]]];
}

void plat_run_window() {
  [NSApp run];
  if(global_pool) [global_pool release];
}
