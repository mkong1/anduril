# MK's Anduril 2 fork

## Differences from OS:
Running feature changes available in CHANGELOG

The base branch is up-to-date as of Rev 642 (2021-12-13) in open source.

## Flash your light with an existing hex file:
I've put the hex files and basic scripts for AVRDude for this Anduril 2 fork in the `hex_files` directory.
If you're looking for the open source Anduril 2 hex files (current as of 2021/11/29), they're in the `open_source_hex_files` directory.

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

from the hex_files directory, run flash_move_hex.sh to move them into the hex_files directory if you wish.

## MK CLI:

If you want to use the CLI I wrote to simplify this process:
  1. make code changes in `/flashlight-firmware/anduril2/ToyKeeper/spaghetti-monster/anduril`
  2. run `code/flash_build.sh`.  This will create your hex files and move them to the `hex_files` directory.
  3. if you have a D4V2, D4SV2, KR4, or KR1, you can use `hex_files/flash_light.sh -h` to find the light in that list, and build it by running `flash_light.sh -l <option>`.  If not, use `flash_light.sh -f <filename>`.

### Caveats:
  (using `flash_light.sh -l` with the appropriate light will select the right hex file for you)
  1. use `noctigon-kr4-nofet.hex` for KR1, anything with a E21A LEDs.
  2. use `noctigon-kr4.hex` for a D4V2 using a KR4 driver.
  3. using `flash_light.sh -f` is currently hard-coded to use `-p t1634`, which works with Emisar/Noctigon lights, but possibly not others.  You may need to adjust this for your own purposes.

### References:
  1. https://budgetlightforum.com/node/68263 for how to flash
  2. Emisar/Noctigon reflashing kit: https://intl-outdoor.com/reflashing-kits.html
  3. Anduril 2 open source hex files from ToyKeeper (including `anduril-manual.txt`): http://toykeeper.net/torches/fsm/anduril2/
  4. Anduril 2 source code: https://launchpad.net/flashlight-firmware
