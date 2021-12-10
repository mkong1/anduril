# basic_off_time_driver
Flashlight driver firmware with ramping using off-time to 
switch modes on attiny13 nanjg drivers.

The flashlight driver has 6 modes: 
high, medium, low, moonlight, smooth ramp, ramp selection

The driver has optional mode memory, enabled if MODE_MEMORY is defined. 
To change modes the user must turn the flashlight off then on in less 
than about 500ms (off-time mode switching), such as by quickly pressing
the switch halfway. The smooth ramp mode fades the light in and out. A 
short press puts the light into ramp selection mode, where the
brightness is the level selected in the ramp mode.

Previously off-time mode switching was not possible without hardware
modifications (such as adding a capacitor to a spare pin of the 
attiny). The method presented in this firmware can be used on a stock 
nanjg driver.

Off-time mode switching is implemented by storing a flag in an area of 
memory that does not get initialized. There is enough energy stored in 
the decoupling capacitor to power the SRAM and keep data during power 
off for about 500ms.

When the firmware starts a byte flag (decay) is checked. If the flashlight
was off for less than ~500ms all bits will still be 0. If the
flashlight was off longer than that some of the bits in SRAM will
have decayed to 1. After the flag is checked it is reset to 0.
Being off for less than ~500ms means the user half-pressed the
switch (using it as a momentary button) and intended to switch modes.

This can be used to store any value in memory. Checking that no
bits in the flag have decayed acts as a checksum on the flag and seems 
to be enough to be reasonably certain other SRAM data is still valid.

In order for this to work brown-out detection must be enabled by
setting the correct fuse bits. I'm not sure why this is, maybe
reduced current consumption due to the reset being held once the
capacitor voltage drops below the threshold?

The smooth ramping mode uses the ram retention method described above.
Without ram retention, the ramp selection would rely on the eeprom. The
large number of writes required by the smooth ramp could cause the 
eeprom to fail if left on for an extended amount of time.

Example of working fuse bit configuration avrdude arguments:
-U lfuse:w:0x79:m -U hfuse:w:0xed:m 
