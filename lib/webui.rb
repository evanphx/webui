require 'ext/webui_platform'

class WebUI
  class Window
    def load_file(file)
      load_url "file://#{File.expand_path file}"
    end

    def get_property(name)
      puts "JS asked for '#{name}'"
      "this is from ruby"
    end

    def close
      WebUI.stop
    end
  end
end
