God.watch do |w|
  w.name = 'bramnik'
  w.start = 'ruby /home/pi/bramnik/software/host/host.rb'
  w.keepalive
end