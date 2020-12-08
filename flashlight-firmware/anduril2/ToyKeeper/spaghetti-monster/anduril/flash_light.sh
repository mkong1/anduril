#!/bin/sh

usage="\nUsage: `basename $0` -fhl
  Use -l if the light is listed below, otherwise use -f to use the hex file
  Examples:
  `basename $0` -f anduril.noctigon-kr4-nofet.hex
  `basename $0` -l 3 (will use same file as above for a KR1)
  -h show this help text
  -l select the number for your flashlight (if present)
    1. D4V2
    2. KR4
    3. KR1
    4. D4V2/KR4 with E21A led
    5. D4SV2
  -f hex filename (hard-coded with `-p t1634`, might not work with your light!)"

while getopts ':hlf:' option; do
  case "$option" in
    h) echo "$usage"
       exit;;
    l) echo "flashing known config for your "
       case $2 in
         1) echo "D4V2"
           avrdude -c usbasp -p t1634 -u -Uflash:w:anduril.emisar-d4v2.hex
           exit;;
         2) echo "KR4 or D4V2 with KR4 driver"
           avrdude -c usbasp -p t1634 -u -Uflash:w:anduril.noctigon-kr4.hex
           exit;;
         3|4) echo "KR1 or D4V2/KR4 with E21A LEDs"
           avrdude -c usbasp -p t1634 -u -Uflash:w:anduril.noctigon-kr4-nofet.hex
           exit;;
         5) echo "D4SV2"
           avrdude -c usbasp -p t1634 -u -Uflash:w:anduril.emisar-d4sv2.hex
           exit;;
       esac
       exit;;
    f) echo "flashing ${2} now"
       file=$2
       avrdude -c usbasp -p t1634 -u -Uflash:w:"${file}"
       exit;;
  esac
done
shift $((OPTIND - 1))

