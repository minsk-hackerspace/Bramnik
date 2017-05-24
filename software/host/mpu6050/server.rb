require 'sinatra'
require_relative 'mpu6050'
require_relative 'hcsro4'

configure do
  set :mpu, MPU6050.new('/home/pi/wiringPi/wiringPi/libwiringPi.so.2.44')
  # set :hc, HCSRO4.new('/home/pi/wiringPi/wiringPi/libwiringPi.so.2.44')
end

get '/' do
  response['Access-Control-Allow-Origin'] = '*'
  settings.mpu.measure.to_s #+ ' ' + settings.hc.measure(17, 27).to_s
end
