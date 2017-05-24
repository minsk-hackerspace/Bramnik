class KalmanFilter
  attr_reader :x0, :p0, :f, :q, :r, :h
  attr_accessor :state, :covariance

  def initialize(q, r, f = 1, h = 1)
    @q = q
    @r = r
    @f = f
    @h = h
  end

  def correct(data)
    # // time update - prediction
    @x0 = @f * @state
    @p0 = @f * @covariance * @f + @q

    # //me asurement update - correction
    k = @h*@p0/(@h*@p0*@h + @r)
    @state = @x0 + k*(data - @h*@x0)
    @covariance = (1 - k*@h)*@p0
  end
end


require 'open-uri'
require 'pp'

x_data = []
y_data = []
100.times do
  data = open 'http://192.168.1.6:4567'
  rotation = data.read.split(' ').map { |d| d.to_f }
  x_data << rotation[0]
  y_data << rotation[1]
  sleep 0.01
end

# pp sensor_data

filtered_x = []

kalman_x = KalmanFilter.new(1, 1, 2, 15) #F, H, Q и R
kalman_x.state = x_data[0]
kalman_x.covariance = 0.1

x_data.each do |d|
  kalman_x.correct d #; // Применяем алгоритм
  filtered_x.push kalman_x.state #; // Сохраняем текущее состояние
end

p '-----------'

require 'pp'
x_data.each_index do |i|
  p [x_data[i], filtered_x[i], y_data[i]]

end
