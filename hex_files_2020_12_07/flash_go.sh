#!/bin/sh

echo $1
file=$1

avrdude -c usbasp -p t1634 -u -Uflash:w:"${file}"
