# MK's Anduril 2 fork

## Differences from OS:
  1. from lockout, 2 clicks to unlock the light.
  2. from lockout, 2 click-hold to unlock at ramp floor.
  3. from lockout, 3 click-hold to unlock at ramp ceiling.
  4. from off and ramp mode, 3 clicks to lock.
  5. from off, 4 clicks to do battery check.
  6. from ramp mode, 4 clicks to change ramp setting (smooth vs stepped).
  7. momentary mode is disabled.

## Just flash my version to your light:
I've put the hex files and basic scripts for AVRDude for the easily-flashed lights I have in the hex_files_2020_12_07 directory.

If you're on a Mac or Linux and use Homebrew:
  1. `brew install avrdude`
  2. `brew install avr-gcc` (only if you plan to make your own changes)

using your flash kit, while holding the pogo-pins to the head of your light: 
  1. run `flash_test.sh` to verify communication with the light.
  2. run `flash_go.sh <filename>` to flash a specific firmware version to your light.

## Making your own changes:
all the code lives in `/flashlight-firmware/anduril2/ToyKeeper/spaghetti-monster/anduril`

run `build-all.sh` in that directory to generate `.hex` files.  This will generate `.hex` files for you, which you can then flash.
  
### Caveats:
  1. use `noctigon-kr4-nofet.hex` for KR1, anything with a E21A LEDs.
  2. use `noctigon-kr4.hex` for a D4V2 using a KR4 driver.

### References:
  1. https://budgetlightforum.com/node/68263 for how to flash
  2. Emisar/Noctigon reflashing kit: https://intl-outdoor.com/reflashing-kits.html
  3. Anduril2 branch from ToyKeeper (including `anduril-manual.txt`): http://toykeeper.net/torches/fsm/anduril2/
