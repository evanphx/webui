require 'webui_platform'

WebUI.create_window 500, 500
WebUI.load_url "http://google.com"
WebUI.run

puts "done!"

p WebUI.run_js("location.href")
