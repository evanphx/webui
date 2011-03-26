require 'mkmf'

$LIBS << "-framework AppKit -framework WebKit"

have_func "rb_thread_blocking_region"

create_makefile("webui_platform")
