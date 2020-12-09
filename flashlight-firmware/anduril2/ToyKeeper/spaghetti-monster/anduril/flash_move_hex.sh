#!/bin/sh

# run from hex_files directory

rm -f *.hex
mv ../code/*.hex .
rm -f anduril.o code/
rm -f anduril.elf code/
