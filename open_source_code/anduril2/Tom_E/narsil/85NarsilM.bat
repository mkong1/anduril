rem 85NarsilM - downloads NarsilM (Tiny85 Multi-channel e-switch UI configurable)
rem
avrdude -p attiny85 -c usbasp -u -Uflash:w:\Tiny254585Projects\NarsilMulti\Release\NarsilM%1.hex:a
