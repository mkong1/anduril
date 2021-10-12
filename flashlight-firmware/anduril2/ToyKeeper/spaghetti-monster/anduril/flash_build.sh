#!/bin/sh

# run from anduril code directory

rm -f ../../../../../hex_files/*.hex
./build-all.sh
mv *.hex ../../../../../hex_files/
rm -f anduril.o
rm -f anduril.elf
