require 'i2c'

class I2Cg
  def initialize(address = 0x68)
    @address = address
    @i2c = ::I2C.create('/dev/i2c-1')
  end

  def read(n = 1)
    @i2c.read(@address, n)
  end

  def write(message)
    @i2c.write(@address, message)
  end
end

i2c = I2Cg.new
case ARGV[0]
  when 'read'
    p 'read'
    p i2c.read ARGV[1].to_i || 1
  else
    puts "write #{ARGV[1]}"
    puts i2c.write(ARGV[1].to_i)
end