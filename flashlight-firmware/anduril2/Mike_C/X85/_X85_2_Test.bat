@ECHO OFF
CD Debug
:START
CLS
ECHO  --=* TEST *=--
ECHO.

avrdude -p t85 -c usbasp -n

ECHO.
PAUSE
GOTO START
