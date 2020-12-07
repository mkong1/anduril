REM BOD disabled:
REM avrdude -p t85 -c usbasp -Ulfuse:w:0xe2:m -Uhfuse:w:0xdf:m -Uefuse:w:0xff:m

REM BOD enabled at 1.8V:
avrdude -p t85 -c usbasp -Ulfuse:w:0xe2:m -Uhfuse:w:0xde:m -Uefuse:w:0xff:m

