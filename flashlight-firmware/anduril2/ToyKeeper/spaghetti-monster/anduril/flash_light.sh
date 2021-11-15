#!/bin/sh

usage="\nUsage: `basename $0` -fhl
  Use -l if the light is listed below, otherwise use -f to use the hex file
  Examples:
  `basename $0` -f anduril.noctigon-kr4-nofet.hex
  `basename $0` -l 3 (will use same file as above for a KR1)
  -h show this help text
  -l select the number for your flashlight (if present)
    1. D4V2
    2. KR4 and D4V2 with KR4 driver
    3. KR1 and D4V2/KR4 with E21A LEDs
    4. D4SV2
    5. D4SV2 with tintramp
    6. MF01S
    7. BLF LT1
    8. D4V2 with 219b LEDs
    9. FW3A nofet
    10. FW3A with 219b/c LEDs
    11. FW3A with full FET
  -f hex filename (hard-coded with -p t1634, might not work with your light!)"

while getopts ':hlf:' option; do
  case "$option" in
    h) echo "$usage"
       exit;;
    l) echo "flashing known config for your "
       case $2 in
         1) echo "D4V2"
           avrdude -c usbasp -p t1634 -u -Uflash:w:anduril.emisar-d4v2.hex
           exit;;
         2) echo "KR4 and D4V2 with KR4 driver"
           avrdude -c usbasp -p t1634 -u -Uflash:w:anduril.noctigon-kr4.hex
           exit;;
         3) echo "KR1 and D4V2/KR4 with E21A LEDs"
           avrdude -c usbasp -p t1634 -u -Uflash:w:anduril.noctigon-kr4-nofet.hex
           exit;;
         4) echo "D4SV2"
           avrdude -c usbasp -p t1634 -u -Uflash:w:anduril.emisar-d4sv2.hex
           exit;;
         5) echo "D4SV2 tintramp"
           avrdude -c usbasp -p t1634 -u -Uflash:w:anduril.emisar-d4sv2-tintramp.hex
           exit;;
         6) echo "MF01S"
           avrdude -c usbasp -p t85 -u -Uflash:w:anduril.mateminco-mf01s.hex
           exit;;
         7) echo "BLF LT1"
           avrdude -c usbasp -p t85 -u -Uflash:w:anduril.blf-lantern.hex
           exit;;
         8) echo "D4V2 and KR4 with 219b"
           avrdude -c usbasp -p t1634 -u -Uflash:w:anduril.noctigon-kr4-219b.hex
           exit;;
         9) echo "FW3A nofet"
           avrdude -c usbasp -p t85 -u -Uflash:w:anduril.fw3a-nofet.hex
           exit;;
         10) echo "FW3A 219"
           avrdude -c usbasp -p t85 -u -Uflash:w:anduril.fw3a-219.hex
           exit;;
         11) echo "FW3A FET"
           avrdude -c usbasp -p t85 -u -Uflash:w:anduril.fw3a.hex
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

