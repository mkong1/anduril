# MK's Anduril 2 fork

## Differences from OS:
  1. from lockout, 2 clicks to unlock the light.
  2. from lockout, 2 click-hold to unlock at ramp floor.
  3. from lockout, 3 clicks to unlock at ramp ceiling.
  4. from off and ramp mode, 3 clicks to lock.
  5. from off, 4 clicks to do battery check.
  6. from ramp mode, 4 clicks to change ramp setting (smooth vs stepped).
  7. from ramp, 2 clicks goes to turbo.
  8. blinks in ramp disabled.
  9. momentary mode is disabled.
  10. tactical strobe disabled.

## Flash your light with an existing hex file:
I've put the hex files and basic scripts for AVRDude for the easily-flashed lights I own in the `hex_files_2020_12_07` directory.

If you're on a Mac or Linux and use Homebrew:
  1. `brew install avrdude`

using your flash kit, while holding the pogo-pins to the head of your light: 
  1. run `flash_test.sh` to verify communication with the light.
  2. run `flash_light.sh -h` to see specific lights with shortcuts to use with `-l`, or `-f` to point to a specific file.

## Making your own changes:

If you're on a Mac or Linux and use Homebrew:
  1. `brew tap osx-cross/avr`
  2. `brew install avr-gcc` (only if you plan to make your own changes)
  
make your changes in `/flashlight-firmware/anduril2/ToyKeeper/spaghetti-monster/anduril`

run `build-all.sh`.  This will generate `.hex` files for you, which you can then flash to your light.
  
### Caveats:
  (using `flash_light.sh -l` with the appropriate light will select the right hex file for you)
  1. use `noctigon-kr4-nofet.hex` for KR1, anything with a E21A LEDs.
  2. use `noctigon-kr4.hex` for a D4V2 using a KR4 driver.

### References:
  1. https://budgetlightforum.com/node/68263 for how to flash
  2. Emisar/Noctigon reflashing kit: https://intl-outdoor.com/reflashing-kits.html
  3. Anduril2 branch from ToyKeeper (including `anduril-manual.txt`): http://toykeeper.net/torches/fsm/anduril2/
