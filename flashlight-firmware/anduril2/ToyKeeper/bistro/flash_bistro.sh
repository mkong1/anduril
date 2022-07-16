#!/bin/sh

avrdude -c usbasp -p t25 -u -Uflash:w:bistro.hex
