@echo off
REM ************* EDIT HERE ******************

rem #low C2, D2, E2 for 0ms, 4ms and 64ms startup. > nul
rem #fuses: high DD disabled DE 1.8V DF 2.7V  > nul


SET lfuse=0xD2
SET hfuse=0xDE
SET efuse=0xff
SET mcu=attiny25

REM ********************** DON'T EDIT BELOW HERE*********************

echo percent one
echo %1
echo "%~1%"
echo %~dp0

if ["%~1%"]==[""]  (
  assoc .hex=hexfile
  echo assoc %0
  reg import "%~dp0clearhexassoc.reg"
  reg add HKEY_CLASSES_ROOT\Applications\%~n0.bat\shell\open\command /ve /d "%~0 ""%%1""" /t REG_SZ /f
  reg add HKEY_CLASSES_ROOT\.hex /ve /d "hexfile" /t REG_SZ /f
  reg add HKEY_CLASSES_ROOT\hexfile\Shell\Open\Command /ve /d "%~0" /t REG_SZ /f
  reg add HKEY_LOCAL_MACHINE\SOFTWARE\Classes\.hex /ve /d "hexfile" /t REG_SZ /f
  reg add HKEY_LOCAL_MACHINE\SOFTWARE\Classes\hexfile\Shell\Open\Command /ve /d "%~0 ""%%1""" /t REG_SZ /f
  reg add HKEY_CURRENT_USER\SOFTWARE\Classes\.hex /ve /d "hexfile" /t REG_SZ /f
  reg add HKEY_CURRENT_USER\SOFTWARE\Classes\hexfile\Shell\Open\Command /ve /d "%~0 ""%%1""" /t REG_SZ /f
rem  reg add HKEY_CURRENT_USER\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.hex\UserChoice /v "ProgId" /d "Applications\%~n0.bat" /t REG_SZ /f
  ftype hexfile=%0 "%1" 

@echo on
@ECHO.
@ECHO.
@ECHO Hex files associated.  Double click any hex to flash
@ECHO.
@ECHO.

goto end
)


SET width=180
SET height=40
SET bufheight=500
SET bufwidth=500

mode con: cols=%width% lines=%height%
powershell -command "&{$H=get-host;$W=$H.ui.rawui;$B=$W.buffersize;$B.width=%bufwidth%;$B.height=%bufheight%;$W.buffersize=$B;}"

SET file=%~d1%~p1%~n1.hex

REM Reconstruct the elf file:

del tmp.elf tmp.bin tmp.hex > nul
avr-objcopy -I ihex -O elf32-avr "%file%" tmp.elf > nul
avr-objcopy -I ihex -O binary "%file%" tmp.bin > nul
avr-objcopy -I binary -O elf32-avr tmp.bin tmp.elf > nul

copy "%file%" tmp.hex > nul

@echo on
avrdude -p %mcu% -c usbasp -u -e
avrdude -c usbasp -p %mcu% -U lfuse:w:%lfuse%:m -U hfuse:w:%hfuse%:m -U efuse:w:%efuse%:m 
avrdude -c usbasp -p %mcu% -U flash:w:tmp.hex
avr-size -C --mcu=%mcu% tmp.elf

@del tmp.elf tmp.bin tmp.hex 

:end
@ECHO This window will self destruct in 45 seconds:
@ECHO.
@ECHO.

@ping -n 45 127.0.0.1 > nul
