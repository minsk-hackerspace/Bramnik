require 'open-uri'
require 'pp'

sensor_data = []
100.times do
  data = open 'http://192.168.1.6:4567'
  sensor_data << data.read.split(' ').map(&:to_f )
  sleep 0.01
end

pp sensor_data