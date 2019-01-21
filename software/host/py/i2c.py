import click
from smbus2 import SMBus, i2c_msg, SMBusWrapper
import time
address = 0x68
offset = 0

#/** 0x10 disable NFC reader + disable keypad*/
#/** 0x11 enable NFC reader + disable keypad*/
#/** 0x12 disable NFC reader + enable keypad*/
#/** 0x13 enable NFC reader + enable keypad*/
#/** 0x20 play "access denied"*/
#/** 0x21 play "access granted"*/
#/** 0x30 ask for status*/
#/** 0x31 ask for 32 bytes from NFC*/
#/** 0x32 ask for 32 bytes from keypad*/
commands = {
        "PLAY_GRANTED": 0x21,
        "PLAY_DENIED": 0x22,
        "STATUS": 0x30,
        "READ_NFC":0x31,
        "READ_KEYPAD":0x32
        }

@click.group()
def rw():
    pass

@rw.command()
@click.argument("num")
def read(**kwargs):
    print("read")
    num = int(kwargs["num"],0)
    print(num)
    with SMBusWrapper(1) as bus:
        msg = bus.read_i2c_block_data(address, 0, num)
    print(msg)
    pass

@rw.command()
@click.argument("command")
def write(**kwargs):
    print("write")
    cmd = kwargs["command"]
    if cmd in commands:
        cmd = commands[cmd]
    else:
        cmd = int(cmd,0)    
    #print(cmd)
    with SMBusWrapper(1) as bus:
        bus.write_byte_data(address, offset, cmd)

#with SMBusWrapper(1) as bus:
#        time.sleep(0.1)
#        bus.write_byte_data(address, offset, 48)
#        time.sleep(0.1)
#        msg = bus.read_byte(address)
#print(msg)
if __name__ == '__main__':
        rw()
