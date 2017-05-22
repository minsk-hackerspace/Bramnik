Interface:
---------

Reader (i2c slave) <-> Host Connector (i2c master)

Long distance i2c (RJ11-4P4C)


- 1 - SDA black
- 2 - VCC red (+12..15V)
- 3 - GND green
- 4 - SCL yellow


Protocol:
---------

Master to slave:

* 0x10 disable NFC reader + disable keypad
* 0x11 enable NFC reader + disable keypad
* 0x12 disable NFC reader + enable keypad
* 0x13 enable NFC reader + enable keypad
* 0x20 play "access denied"
* 0x21 play "access granted"
* 0x30 ask for status
* 0x31 ask for 32 bytes from NFC
* 0x32 ask for 32 bytes from keypad

slave to master:

* 0 bytes when no ask before request
* 1 byte of status when 0x30 asked (0 = nothing happens, 1 = has NCF data , 2 = has keypad data)
* 32 bytes from NFC when 0x31 asked
* 32 bytes from Keypad when 0x32 asked


Reader Scheme (slave):
-------

![](https://github.com/minsk-hackerspace/Bramnik/raw/master/hardware/reader_scheme.png)


TODO:
-----

add readme
