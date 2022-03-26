# Reader firmware

The firmware waits for event from the NFC reader (door unit), saves code or NFC tag ID to queue and
signals to host about event and send queue contents via I2C. It receives I2C commands (open the door
or play 'access denied' sound) from the host also.

Before compile and flash, change the hardcoded backup door code in the `keypadData_HARDCODED_PASS`
variable to secret value (BCD format). For example, `{0x12, 0x34, 0x56, 0x78}` for 12345678.
