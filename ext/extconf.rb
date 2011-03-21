require 'mkmf'

$LIBS << "-framework AppKit -framework WebKit"

create_makefile("webui_platform")
