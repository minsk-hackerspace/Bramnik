import sys
import codecs
hexify = codecs.getencoder('hex')
import traceback

import logging
#logging.basicConfig(stream=sys.stdout, level=logging.DEBUG)
#logging.basicConfig(stream=sys.stdout, level=logging.WARNING)
logging.basicConfig(
            level=logging.DEBUG,
            format="%(asctime)s %(name)s %(levelname)-8s %(thread)d %(message)s",
            datefmt="%Y-%m-%d %H:%M:%S")

logger = logging.getLogger("bramnik")

from smbus2 import SMBus, i2c_msg, SMBusWrapper
#from smbus import SMBus
bus = SMBus(1) # 0 indicates /dev/i2c-0

import time
import RPi.GPIO as GPIO




# I2C consts
address = 0x68
offset = 0

# Constants
NORMAL_DELAY = 1
EXCEPTION_DELAY = 2
# commands
PLAY_GRANTED = 0x20
PLAY_DENIED = 0x21
STATUS = 0x30
READ_NFC = 0x31
READ_KEYPAD = 0x32

# HW consts
DOOR_GPIO_PIN = 17  #BCM.17

GPIO.setmode(GPIO.BCM)
GPIO.setup(DOOR_GPIO_PIN, GPIO.OUT)


# I2C low level
def read(num):
    #with SMBusWrapper(1) as bus:
    #msg = [bus.read_byte(address) for x in range(0,num)]
    msg = i2c_msg.read(address, num)
    bus.i2c_rdwr(msg)
    return list(msg)

def write(command):
    #with SMBusWrapper(1) as bus:
    bus.write_byte(address, command)
def open_door():
    logger.warning("Openning door")
    GPIO.output(DOOR_GPIO_PIN, 1)
    time.sleep(0.1)
    GPIO.output(DOOR_GPIO_PIN, 0)


# logic
def read_status():
    #return bytearray([1])
    write(STATUS)
    time.sleep(0.1)
    s = read(1)
    return s

# reads NFC and checks if it is valid
def check_nfc():
    logger.debug("checking nfc")
    write(READ_NFC)
    time.sleep(0.1)
    s = read(16)
    logger.warning("checking nfc: %s", s)
    if s[:3] == [131, 78, 213]:
        open_door()
        write(PLAY_GRANTED)
    else:
        logger.warning("access denied for: %s", s)
        write(PLAY_DENIED)

# reads code and checks if it is valid
def check_code():
    logger.debug("checking code")
    write(READ_KEYPAD)
    time.sleep(0.1)
    s = read(16)
    logger.warning("checking code: %s", s)
    if s[:4] == [49,50,51,52]: # 1,2,3,4 on keypad
        open_door()
        write(PLAY_GRANTED)
    else:
        logger.warning("access denied for: %s", s)
        write(PLAY_DENIED)

def main_loop():
    logger.warning("starting main loop")
    delay = NORMAL_DELAY
    while(True):
        try:
            logger.debug("loop")
            s = read_status()
            s = s[0]
            logger.info("status: %s", s)
            if s&1: # lower bit means reader has NFC data
                check_nfc()
            if s&2: # second bit means reader has code data
                check_code()

        except Exception as e:
            logger.error("Exception: %s", e)
            logger.error(traceback.format_exc())
            delay = EXCEPTION_DELAY
        else:
            delay = NORMAL_DELAY
        #delay
        time.sleep(delay)




main_loop()
