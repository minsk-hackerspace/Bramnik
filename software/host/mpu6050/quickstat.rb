require 'quickstats'
require 'pp'

rx = []
ry = []

File.open 'set.txt', 'r' do |f|
  str = f.readlines.join.gsub("\n", '')
  begin
    m = str.match /\d+.\d+/
    break if m.nil?
    rx << m.to_s.to_f
    str = m.post_match

    m = str.match /\d+.\d+/
    ry << m.to_s.to_f
    str = m.post_match

  end until m.nil?

  k = 0.15

  rx_opt = []
  rx_opt.push rx.first
  (rx.size-1).times do |i|
    rx_opt << (k * rx[i+1] + (1-k) * rx_opt[i]).round(1)
  end

  ry_opt = []
  ry_opt.push ry.first
  (ry.size-1).times do |i|
    ry_opt << (k * ry[i+1] + (1-k) * ry_opt[i]).round(1)
  end

  require 'flotr'
# Create a new plot
  plot = Flotr::Plot.new("Test plot")

  raw_x = Flotr::Data.new(:label => "rotation x", :color => "green")
rx.each_with_index { |x, i| raw_x << [i, x]}

  rotation_x = Flotr::Data.new(:label => "rotation x", :color => "red")
  rx_opt.each_with_index do |x, i|
    rotation_x  <<  [i, x]
  end

  rotation_y = Flotr::Data.new(:label => "rotation y", :color => "blue")
  ry_opt.each_with_index do |y, i|
    rotation_y << [i, y]
  end


  plot << rotation_x << raw_x #<< rotation_y
  plot.show

  p rx
  p rx_opt
  p '--'
  p ry
  p ry_opt
end