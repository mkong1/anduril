@ECHO OFF
CD Debug
:START
CLS
ECHO  --=* TEST *=--
ECHO.
avrdude -p t85 -c usbasp -n
ECHO.
ECHO PRESS ENTER TO FLASH!
ECHO.
PAUSE
CLS

ECHO  --=* FLASH *=--
ECHO.

avrdude -p t85 -c usbasp -u -U flash:w:X85.hex:i -U lfuse:w:0xe2:m -U hfuse:w:0xde:m -U efuse:w:0xff:m

ECHO.
PAUSE
GOTO START
