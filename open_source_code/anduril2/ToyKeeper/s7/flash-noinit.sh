#/bin/sh
FIRMWARE=$1
avrdude -c usbasp -p t13 -u -Uflash:w:$FIRMWARE -Ulfuse:w:0x79:m -Uhfuse:w:0xed:m
