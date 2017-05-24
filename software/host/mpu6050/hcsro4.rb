require 'fiddle'
require 'inline'

class HCSRO4
  IN = 0
  OUT = 1

  TRIG = 17
  ECHO = 27

  def initialize(path_to_wiring_pi_so)
    wiringpi = Fiddle.dlopen(path_to_wiring_pi_so)

    int = Fiddle::TYPE_INT
    void = Fiddle::TYPE_VOID

    # extern int  wiringPiSetup       (void) ;
    @setup = Fiddle::Function.new(wiringpi['wiringPiSetup'], [void], int)

    # extern int  wiringPiSetupGpio       (void) ;
    @setup_gpio = Fiddle::Function.new(wiringpi['wiringPiSetupGpio'], [void], int)

    # extern void pinMode             (int pin, int mode) ;
    @pin_mode = Fiddle::Function.new(wiringpi['pinMode'], [int, int], void)

    @setup_gpio.call nil
    @pin_mode.call TRIG, OUT
    @pin_mode.call ECHO, IN
  end

  inline do |builder|
    #sudo cp WiringPi/wiringPi/*.h /usr/include/
    builder.include '<wiringPi.h>'
    builder.c '
    double measure(int trig, int echo){
        //initial pulse
        digitalWrite(trig, HIGH);
        delayMicroseconds(20);
        digitalWrite(trig, LOW);

        //Wait for echo start
        while(digitalRead(echo) == LOW);

        //Wait for echo end
        long startTime = micros();
        while(digitalRead(echo) == HIGH);

        long travelTime = micros() - startTime;
        double distance = travelTime / 58.0;

        return distance;
    }
  '
  end
end