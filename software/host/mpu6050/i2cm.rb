require 'fiddle'


class I2Cm
  attr_accessor :fd, :i2c_read, :i2c_write

  def initialize(path_to_wiring_pi_so='/home/pi/wiringPi/wiringPi/libwiringPi.so.2.44')
    wiringpi = Fiddle.dlopen(path_to_wiring_pi_so)

    int = Fiddle::TYPE_INT
    char_p = Fiddle::TYPE_VOIDP

    # int wiringPiI2CSetup (int devId) ;
    @i2c_setup = Fiddle::Function.new(wiringpi['wiringPiI2CSetup'], [int], int)

    # int wiringPiI2CSetupInterface (const char *device, int devId) ;
    @i2c_setup_interface = Fiddle::Function.new(wiringpi['wiringPiI2CSetupInterface'], [char_p, int], int)

    # int wiringPiI2CRead (int fd) ;
    @i2c_read = Fiddle::Function.new(wiringpi['wiringPiI2CRead'], [int], int)

    # int wiringPiI2CWrite (int fd, int data) ;
    @i2c_write = Fiddle::Function.new(wiringpi['wiringPiI2CWrite'], [int, int], int)

    # int wiringPiI2CWriteReg8 (int fd, int reg, int data) ;
    @i2c_write_reg8 = Fiddle::Function.new(wiringpi['wiringPiI2CWriteReg8'], [int, int, int], int)

    # int wiringPiI2CWriteReg16 (int fd, int reg, int data) ;
    @i2c_write_reg8 = Fiddle::Function.new(wiringpi['wiringPiI2CWriteReg16'], [int, int, int], int)

    # int wiringPiI2CReadReg8 (int fd, int reg) ;
    @i2c_read_reg8 = Fiddle::Function.new(wiringpi['wiringPiI2CReadReg8'], [int, int], int)

    # int wiringPiI2CReadReg16 (int fd, int reg) ;
    @i2c_read_reg16 = Fiddle::Function.new(wiringpi['wiringPiI2CReadReg16'], [int, int], int)

    @fd = @i2c_setup.call 0x68
  end

  def write(value)
    @i2c_write.call(@fd, value)
  end

  def read
    @i2c_read.call(@fd)
  end
end

i2c = I2Cm.new
case ARGV[0]
  when 'read'
    puts 'read'
    puts i2c.read
  else
    puts "write #{ARGV[1]}"
    puts i2c.write(ARGV[1].to_i)
end