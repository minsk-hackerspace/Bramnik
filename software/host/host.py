#!/usr/bin/python3

import sys
import codecs
hexify = codecs.getencoder('hex')
import datetime
import traceback
import RPi.GPIO as GPIO
import threading
import signal
import requests
import json
from models import *

import argparse
_LOG_LEVEL_STRINGS = ['CRITICAL', 'ERROR', 'WARNING', 'INFO', 'DEBUG']

import logging
logging.basicConfig(
            level=logging.DEBUG,
            format="%(asctime)s %(name)s %(levelname)-8s %(message)s",
            datefmt="%Y-%m-%d %H:%M:%S")

logger = None

from smbus2 import SMBus, i2c_msg
bus = SMBus(1) # 0 indicates /dev/i2c-0

import time

# I2C consts
address = 0x06
offset = 0

READER_IRQ_PIN = 4

# commands from WG card reader
EVENT_CARD_ID = 0
EVENT_CODE    = 1
EVENT_ERROR   = 2
EVENT_STATE   = 3
EVENT_DOOR    = 4
EVENT_NODATA  = 0xfe

# commands to WG card reader
CMD_BEEP       = (1<<1)
CMD_GREENLED   = (1<<2)
CMD_READER_EN  = (1<<3)
CMD_OPEN       = (1<<4)
CMD_DENY       = (1<<5)

got_USR1 = False

codes_to_check = list()
cards_to_check = list()
# I2C low level
# ---------------------------------------------------------
def read(num):
    msg = i2c_msg.read(address, num)
    bus.i2c_rdwr(msg)
    return list(msg)

def write(command):
    bus.write_byte(address, command)

def open_door():
    logger.warning("Openning door")
    write(CMD_READER_EN|CMD_GREENLED|CMD_OPEN)

def deny_access():
    logger.warning("Deny access")
    write(CMD_READER_EN|CMD_DENY)

# arg parsing
# ---------------------------------------------------------
def _log_level_string_to_int(log_level_string):
    if not log_level_string in _LOG_LEVEL_STRINGS:
        message = 'invalid choice: {0} (choose from {1})'.format(log_level_string, _LOG_LEVEL_STRINGS)
        raise argparse.ArgumentTypeError(message)

    log_level_int = getattr(logging, log_level_string, logging.INFO)
    assert isinstance(log_level_int, int)
    return log_level_int

def notify_telegram(message):
    global tg_chat_id
    global tg_token

    url = 'https://api.telegram.org/bot{0}/sendMessage'.format(tg_token)
    data = {"chat_id": tg_chat_id, "text": message}

    try:
        x = requests.post(url, data = data)
    except Exception as e:
        logger.error("fail to post notification: %s", e)

# works as interrupt
def reader_request_callback(gpio):
    global codes_to_check
    global cards_to_check
    global reader_event

    while True:
        data = read(6)
        event = data[0]
        print(event)
        if event == EVENT_NODATA:
            break

        #bits = data[1]
        #bytes = int((bits + 7) / 8)
        #print(str(bytes) + " bytes")
        #for i in range(0, bytes):
        #    print(hex(data[i + 2]))

        # remember what happens (it is interrupt).

        # Card byte order is reversed
        if event == EVENT_CARD_ID:
            cards_to_check.append(data[-1:1:-1])
        if event == EVENT_CODE:
            codes_to_check.append(data[-1:1:-1])

    reader_event.set()


# reads NFC and checks if it is valid
def check_nfc(card_code):
    nfc_str = ''.join('{:02x}'.format(x) for x in card_code)
    logger.warning("checking nfc: %s", nfc_str)

    try:
        cards = Card.select().join(User).where(Card.card_id == nfc_str)
        if len(cards)==0:
            raise Exception("Card has no valid user")
        card = cards[0]

        logger.warning("checking user %s", card.user_id.name)

        if card.user_id.valid_till < datetime.datetime.now():
            raise Exception("User exists but access expired")

        if not card.user_id.access_allowed:
            logger.error("Acces denied for user %s", card.user_id.name)
            raise Exception()

        open_door()
        notify_telegram(u'Дверь открыта участником №{0} ({1})'
                .format(card.user.account_id, card.user.name))


    except Exception as e:
        logger.error("unauthorized card read: %s", nfc_str)
        logger.debug(traceback.format_exc())
        deny_access()

def code_to_str(code):
    str_code = ''
    for a in reversed(code):
        if a >> 4 == 15:
            break

        str_code += str(a >> 4)

        if a & 0x0f == 15:
            break

        str_code += str(a & 0x0f)

    return str_code


# reads code and checks if it is valid
def check_code(code):

    logger.warning("checking code: %s", code)

#    code_str = ''.join(map(lambda c: str(c & 0x0f) + str(c >> 4), code))[::-1]
    code_str = code_to_str(code)

    try:
        codes = Code.select().where(Code.code == code_str)
        if len(codes)==0:
            raise Exception("Intruder alert! Wrong code")
        code = codes[0]
        if code.valid_till < datetime.datetime.now():
            raise Exception("Code exists but expired")
        logger.error("opening door with code %s", code_str)
        open_door()
        notify_telegram('Дверь открыта кодом {0} (участник №{1}, {2})'
                .format(code_str, code.user.account_id, code.user.name))

    except Exception as e:
        logger.error("unauthorized code read: %s", code_str)
        logger.debug(traceback.format_exc())
        deny_access()


def main_loop():
    global codes_to_check
    global cards_to_check
    global reader_event
    global got_USR1

    reader_event = threading.Event()
    reader_event.clear()

    logger.warning("starting main loop")

    #setup interrupt callback on GPIO
    GPIO.setmode(GPIO.BCM)
    GPIO.setup(READER_IRQ_PIN, GPIO.IN, pull_up_down=GPIO.PUD_UP)
    GPIO.add_event_detect(READER_IRQ_PIN, GPIO.FALLING, callback=reader_request_callback)
    if GPIO.input(READER_IRQ_PIN) == GPIO.LOW:
        reader_request_callback(READER_IRQ_PIN)

    while(True):
        try:
            # check if codes arrived and handle them
            for nfc in cards_to_check:
                check_nfc(nfc)
            cards_to_check = []
            for code in codes_to_check:
                check_code(code)
            codes_to_check = []

            if got_USR1:
                logger.warning("Got signal USR1 — open the door")
                open_door()
                got_USR1 = False

        except Exception as e:
            logger.error("Exception: %s", e)
            logger.debug(traceback.format_exc())

        reader_event.wait(timeout=10)
        reader_event.clear()


def main():
    global logger
    global tg_token
    global tg_chat_id
    parser = argparse.ArgumentParser()

    parser.add_argument('--loglevel',
                        default='INFO',
                        dest='loglevel',
                        type=_log_level_string_to_int,
                        nargs='?',
                        help='Set the logging output level. {0}'.format(_LOG_LEVEL_STRINGS))
    parser.add_argument('--tgtoken',
                        dest='tgtoken',
                        nargs='?',
                        help='Telegram bot token for notifications')
    parser.add_argument('--tgchatid',
                        dest='tgchatid',
                        nargs='?',
                        help='Telegram chat id for notifications')
    parsed_args = parser.parse_args()
    logger = logging.getLogger("bramnik")
    logger.setLevel(parsed_args.loglevel)
    logging.getLogger("peewee").setLevel(parsed_args.loglevel)

    tg_token = parsed_args.tgtoken
    tg_chat_id = parsed_args.tgchatid

    db.connect()
    logger.warning("Total users: %d", User.select().count())
    logger.warning("Total cards: %d", Card.select().count())

    main_loop()
    GPIO.cleanup()

def USR1_handler(signum, frame):
    global got_USR1
    global reader_event

    got_USR1 = True
    reader_event.set()

if __name__ == '__main__':
    signal.signal(signal.SIGUSR1, USR1_handler)
    main()
