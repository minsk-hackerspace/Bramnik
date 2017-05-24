require 'fiddle'

class MPU6050
  attr_reader :last_x, :last_y, :k
  def initialize(path_to_wiring_pi_so)
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
    @i2c_write_reg8.call @fd, 0x6B, 0x00

    @last_x = 0
    @last_y = 0
    @k = 0.30

  end

  def read_word_2c(fd, addr)
    val = @i2c_read_reg8.call(fd, addr)
    val = val << 8
    val += @i2c_read_reg8.call(fd, addr+1)
    val -= 65536 if val >= 0x8000
    val
  end

  def measure
    gyro_x = (read_word_2c(@fd, 0x43) / 131.0).round(1)
    gyro_y = (read_word_2c(@fd, 0x45) / 131.0).round(1)
    gyro_z = (read_word_2c(@fd, 0x47) / 131.0).round(1)


    acceleration_x = read_word_2c(@fd, 0x3b) / 16384.0
    acceleration_y = read_word_2c(@fd, 0x3d) / 16384.0
    acceleration_z = read_word_2c(@fd, 0x3f) / 16384.0

    rotation_x = k * get_x_rotation(acceleration_x, acceleration_y, acceleration_z) + (1-k) * @last_x
    rotation_y = k * get_y_rotation(acceleration_x, acceleration_y, acceleration_z) + (1-k) * @last_y

    @last_x = rotation_x
    @last_y = rotation_y

    # {gyro_x: gyro_x, gyro_y: gyro_y, gyro_z: gyro_z, rotation_x: rotation_x, rotation_y: rotation_y}
    "#{rotation_x.round(1)} #{rotation_y.round(1)}"
  end

  private
  def to_degrees(radians)
    radians / Math::PI * 180
  end

  def dist(a, b)
    Math.sqrt((a*a)+(b*b))
  end

  def get_x_rotation(x, y, z)
    to_degrees Math.atan(x / dist(y, z))
  end

  def get_y_rotation(x, y, z)
    to_degrees Math.atan(y / dist(x, z))
  end

end