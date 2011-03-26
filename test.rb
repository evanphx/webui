require 'webui'

WebUI.init

w = WebUI.create_window 500, 500
w.load_file "test.html"

w2 = WebUI.create_window 500, 500
w2.load_url "http://google.com"

WebUI.run

puts "done!"

p w.run_js("location.href")
