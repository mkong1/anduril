#!/bin/sh

# run from hex_files directory

rm -f *.hex
mv ../code/*.hex .
rm -f code/anduril.o
rm -f code/anduril.elf
