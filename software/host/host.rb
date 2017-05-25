require 'i2c'
require 'logger'
require 'base64'
require_relative 'user'

class Bramnik
  attr_accessor :logger
  def initialize(address = 0x68, device = '/dev/i2c-1', logfile = '/tmp/bramnik.log')
    @address = address
    @i2c = ::I2C.create(device)
    `gpio mode 0 out`
    @logger = Logger.new(logfile)
  end

  def read(n = 1)
    data = @i2c.read(@address, n)
    @logger.info "Received: #{Base64.encode64 data}" unless data == "\x00"
    data
  end

  def write(data)
    @logger.info "Sent: #{data}" unless data == 48
    @i2c.write(@address, data)
  end

  def status
    write(0x30)
    read 1
  end

  def read_nfc
    write 0x31
    data = Base64.encode64 read 32
    p data
    if User.check_access data
      play_granted
      open_door
    else
      play_denied
    end
    data
  end

  def read_keyboard
    write 0x32
    data = Base64.encode64 read 32
    p data
    if User.check_code data
      play_granted
      open_door
    else
      play_denied
    end
    data
  end

  def disable
    write 0x10
  end

  def enable_reader_disable_keypad
    write 0x11
  end

  def disable_reader_enable_keypad
    write 0x12
  end

  def enable
    write 0x13
  end

  def play_granted
    write 0x21
  end

  def play_denied
    write 0x20
  end

  def open_door
    `gpio write 0 1`
    sleep 0.2
    `gpio write 0 0`
  end
end

bramnik = Bramnik.new

while true
  status = bramnik.status
  case status
    when "\x00"
    when "\x01"
      bramnik.read_nfc
    when "\x02"
      bramnik.read_keyboard
    else
      bramnik.logger.warn "Wrong status #{status}"
  end
  sleep 1
end