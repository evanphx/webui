require 'mkmf'

if RUBY_PLATFORM =~ /linux/
  unless pkg_config "gtk+-2.0"
    puts "where is gtk+?"
    exit 1
  end

  unless pkg_config "webkit-1.0"
    puts "where is webkit?"
    exit 1
  end

  $objs = %w!webui_platform.o webui_gtk.o!
else
  $LIBS << "-framework AppKit -framework WebKit"
  $objs = %w!webui_platform.o webui_mac.o!
end

have_func "rb_thread_blocking_region"

create_makefile("webui_platform")
