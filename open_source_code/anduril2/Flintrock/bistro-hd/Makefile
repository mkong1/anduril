#Makefile for bistro-HD,  
# Flintrock 2017 (with some contributions from atmel studio), GPLV3 I suppose, whatever, basically I don't care.
#

#### Define where your custom firmware configurations are ############
CONFIG_DIR := configs

###### define firmwares to build based on minimum attiny they can run on#############
# Versions for all compatible attiny's will be built unless ONE_BUILD is defined.
# These must correspond to config headers named like config_<FIRMWARE-NAME>.h

FIRMWARES13 += biscotti-HD
# unforutnately BLFA6_EMU doesn't actually fit on an attiny13 yet: 
#FIRMWARES13 += BLFA6_EMU-HD 
FIRMWARES25 += BLFA6_EMU-HD 
FIRMWARES25 += default-HD TAv1-OTC-HD TAv1-OTSM-HD TAv1-OTSM-LDO-HD classic-HD tripledown-HD eswitch-TA-HD
FIRMWARES25 += dual-switch-dumbclick-TA-HD dual-switch-noinit-TA-HD dual-switch-turboclick-TA-HD 4channel-dual-switch-HD
FIRMWARES25 += dual-switch-OTSM-TA-HD eswitch-Q8-fetplusone-HD TAv1-OTC-NODIVIDER-1SONLY-HD eswitch-TA-VCCREAD-INDICATOR-HD
FIRMWARES25 += TAv1-OTSM-LDO-fetonly-HD TAv1-OTSM-fetonly-HD eswitch-TA-FETONLY-VCCREAD-INDICATOR-HD
FIRMWARES45 += 
FIRMWARES85 +=
FIRMWARES := $(FIRMWARES13) $(FIRMWARES25) $(FIRMWARES45) $(FIRMWARES85)

# Uncomment next to override multiple builds and build one specific file. 
#  Must be named like bistr-<config-name>-attinyXX.hex, where XX is a valid attiny model number.
#  (Actually can include multiple individual builds per line too or using += )
#ONE_BUILD := bistro-biscotti-HD-attiny13.hex
#ONE_BUILD := bistro-dual-switch-OTS-attiny25.hex 
#ONE_BUILD := bistro-TAv1-OTSM-HD-attiny25.hex  bistro-4channel-dual-switch-HD-attiny25.hex
#ONE_BUILD := bistro-eswitch-TA-HD-attiny25.hex
#ONE_BUILD := bistro-BLFA6_EMU-HD-attiny13.hex
#ONE_BUILD := bistro-eswitch-Q8-fetplusone-HD-attiny85.hex
#ONE_BUILD := bistro-TAv1-OTC-NODIVIDER-1SONLY-HD-attiny25.hex

#Uncomment next to skip battcheck builds, speeds things up a bit:
#NO_BATTCHECK

########### Define where your compiling programs live #####################

ifneq ($(OS),Windows_NT)
###For linux using files installed directly into / from Atmel
AVR_DIR:=/arduino-1.8.5
GCC_DIR:=$(AVR_DIR)/hardware/tools/avr/bin
INCLUDES:=$(AVR_DIR)/hardware/tools/avr/avr/

else
###For Atmel studio 7.0:
GCC_DIR := C:\Program Files (x86)\Atmel\Studio\7.0\toolchain\avr8\avr8-gnu-toolchain\bin
INCLUDES := C:\Program Files (x86)\Atmel\Studio\7.0\Packs\atmel\ATtiny_DFP\1.1.102\include

###Or using WinAVR version: 20100110 (the latest version but still works)
#GCC_DIR := C:\WinAVR-20100110\bin
#INCLUDES := C:\WinAVR-20100110\avr\include\avr
# That's more outdated than the atmel version but seems to work.

# The next one should work, with atmel's gnu toolchain, available here:http://www.atmel.com/tools/atmelavrtoolchainforwindows.aspx
#GCC_DIR := C:\avr8-gnu-toolchain\bin
#Not sure of the include path..
endif

#Files to pack up to copy it all
PACKAGE_FILES := bistro-HD.c *.h THE_MANUAL.htm license.txt buildall-AS.bat buildall-WinAVR.bat Makefile configs modegroups hex
PACKAGE_FILES += flashany_attiny13.bat flashany_attiny25.bat flashany_attiny45.bat flashany_attiny85.bat clearhexassoc.reg CHANGES.txt

PACKAGE_DIR := C:\firmware\HD
7ZIP := C:\Program Files\7-Zip\7z.exe


#############################You probably don't need to edit below here##############################

ifneq ($(OS),Windows_NT)
  EXE:=
else
  SHELL := cmd.exe
  EXE:=.exe
endif

############Define the output directories###################
HEX_DIR := .\hex
PREPROCESS_DIR := .\Preprocessor-output
# where the .elf files go, just left around for size determination:
BIN_DIR := .\bin

#############Define where all the bits and pieces are #####################
GCC := $(GCC_DIR)/avr-gcc$(EXE)
OBJ_COPY := $(GCC_DIR)/avr-objcopy$(EXE)
OBJ_DUMP := $(GCC_DIR)/avr-objdump$(EXE)
AVR_SIZE := $(GCC_DIR)/avr-size$(EXE)


# The following seems uneccessary
#SPECDIR := C:\Program Files (x86)\Atmel\Studio\7.0\Packs\atmel\ATtiny_DFP\1.1.102\gcc\

##########Compiler Options##################

C_FLAGS := -x c -funsigned-char -funsigned-bitfields -Os -finline-functions-called-once 
C_FLAGS += -ffunction-sections -fdata-sections -fpack-struct -fshort-enums -Wall -std=gnu99 
C_FLAGS += -fverbose-asm
LDD_FLAGS := -Wl,--start-group  -Wl,--end-group -Wl,--gc-sections
#LDD_FLAGS += -Wl,-lm

#### define groups of attinys based on minimum capability ###

ATTINYS13 := 13 25 45 85
ATTINYS25 := 25 45 85
ATTINYS45 := 45 85
ATTINYS85 := 85


#construct combinations of firmwares and attinys to build.

BUILDS13 := $(foreach X,$(FIRMWARES13),$(foreach Y,$(ATTINYS13),bistro-$X-attiny$Y.hex))
BUILDS25 := $(foreach X,$(FIRMWARES25),$(foreach Y,$(ATTINYS25),bistro-$X-attiny$Y.hex))
BUILDS45 := $(foreach X,$(FIRMWARES45),$(foreach Y,$(ATTINYS45),bistro-$X-attiny$Y.hex))
BUILDS85 := $(foreach X,$(FIRMWARES85),$(foreach Y,$(ATTINYS45),bistro-$X-attiny$Y.hex))
BUILDS := $(BUILDS13) $(BUILDS25) $(BUILDS45) $(BUILDS85)

ifdef ONE_BUILD
# override build list
 BUILDS := $(ONE_BUILD)
endif

ALLBUILDS := $(BUILDS)

BATTCHECK_BUILDS := $(BUILDS:bistro%.hex=battcheck%.hex)
# We don't need preprocessor files for battcheck builds, so need a phony target for them.
BATTCHECK_PREPROC := $(BUILDS:bistro%.hex=battcheck%.i)

ifndef NO_BATTCHECK
 ALLBUILDS += $(BATTCHECK_BUILDS)
endif

#$(TARGETS:%.hex=.o)
#	C:\Program Files (x86)\Atmel\Studio\7.0\toolchain\avr8\avr8-gnu-toolchain\bin\avr-gcc.exe 

# A rule to make a generic myprogram.hex  from a file myprogram.c
# This is not really implemented at the moment.  The linker section below still parses the attiny version from the program name anyway.
$(PROGRAMS:%.hex=%.o): ./%.c
	@echo Building file: $@
	"$(GCC)" -c $(C_FLAGS) -I"$(INCLUDES)"  -mmcu=attiny$(ATTINY)   -o "$@" -DATTINY=$(ATTINY)  "$<" 
	@echo Finished building: $@


#These build all the specific firmware-attinyXX.hex combination
# removed this: -B "$(SPECDIR)$(MCU)"  ... seems uneeded and makes compiling with WinAVR difficult
$(BUILDS:%.hex=%.o): ./bistro-HD.c
	@echo Building file: $@
	$(eval MCU=$(lastword $(subst -, ,$(basename $@))))
	$(eval HOWTINY=$(subst attiny,,$(MCU)))
	"$(GCC)" -c $(C_FLAGS) -I"$(INCLUDES)"  -mmcu=$(MCU)   -o "$@" -DCONFIG_FILE_H=\"$(CONFIG_DIR)/config_$(@:bistro-%-$(MCU).o=%.h)\" -DATTINY=$(HOWTINY)  "$<" 
	@echo Finished building: $@

#This makes battcheck versions of each build
$(BATTCHECK_BUILDS:%.hex=%.o): ./bistro-HD.c
	@echo Building file: $@
	$(eval MCU=$(lastword $(subst -, ,$(basename $@))))
	$(eval HOWTINY=$(subst attiny,,$(MCU)))
	"$(GCC)" -c $(C_FLAGS) -I"$(INCLUDES)"  -mmcu=$(MCU)   -o "$@" -DVOLTAGE_CAL="" -DCONFIG_FILE_H=\"$(CONFIG_DIR)/config_$(@:battcheck-%-$(MCU).o=%.h)\" -DATTINY=$(HOWTINY)  "$<" 
	@echo Finished building: $@


# removed this: -B "$(SPECDIR)$(MCU)"
$(BUILDS:%.hex=%.i): ./bistro-HD.c
	@echo Building file: $@
	$(eval MCU=$(lastword $(subst -, ,$(basename $@))))
	$(eval HOWTINY=$(subst attiny,,$(MCU)))
	"$(GCC)" -E -P  $(C_FLAGS) -I"$(INCLUDES)"  -mmcu=$(MCU)   -o "$@" -DCONFIG_FILE_H=\"$(CONFIG_DIR)/config_$(@:bistro-%-$(MCU).i=%.h)\" -DATTINY=$(HOWTINY)  "$<" 
	cat license.txt $@ > temp.txt
	mv temp.txt $@
	@echo Finished building: $@

#phony target that does nothing.
#$(BATTCHECK_PREPROC):;


#This is rule is more more specialized than it appears
#It requires a target file with a name basename-attinyXX.hex and parses the attiny version from the target.
# specifically the names should be like bistro-<config>-attinyXX.hex
# actually this hex builder only requires something-attinyXX.hex so is more general
# but the .o file targets above require bistro-<config> or battcheck-<config>
#removed -B "$(SPECDIR)/$(MCU)" from gcc line
%.hex: %.o %.i
	mkdir  $(HEX_DIR) || exit 0
	mkdir  $(BIN_DIR) || exit 0
	mkdir  $(PREPROCESS_DIR) || exit 0
	$(eval MCU=$(lastword $(subst -, ,$(basename $@))))
	$(eval HOWTINY=$(subst attiny,,$(MCU)))
	@echo Building target: $@
# This part is essentially from atmel's automatic make file, just with abstraction added and the attiny version parsing.
	"$(GCC)" -o "$(basename $@).elf" "$(basename $@).o" $(OBJS_AS_ARGS) $(USER_OBJS) $(LIBS) -Wl,-Map="$(basename $@).map" $(LDD_FLAGS) -mmcu=$(MCU)   
	@echo Finished building $(@:%.hex=%.elf)
	"$(OBJ_COPY)" -O ihex -R .eeprom -R .fuse -R .lock -R .signature -R .user_signatures  "$(basename $@).elf" "$(basename $@).hex"	
	"$(OBJ_COPY)" -j .eeprom  --set-section-flags=.eeprom=alloc,load --change-section-lma .eeprom=0  --no-change-warnings -O ihex "$(basename $@).elf" "$(basename $@).eep" || exit 0	
	"$(OBJ_DUMP)" -h -S "$(basename $@).elf" > $(basename $@).lss
	"$(OBJ_COPY)" -O srec -R .eeprom -R .fuse -R .lock -R .signature -R .user_signatures "$(basename $@).elf" "$(basename $@).srec"
#	"$(AVR_SIZE)" "$(basename $@).elf"
	cp $@ $(HEX_DIR)
	cp $(@:%.hex=%.i) $(PREPROCESS_DIR) || exit 0
	cp $(@:%.hex=%.elf) $(BIN_DIR) || exit 0
	@echo Finished building target: $@

# Need to force the order, can't just use prerequisites.
all: 
	make clean 
	make $(ALLBUILDS) 
	make tidy 
	make sizes 

#tidy cleans everything but the copied hex:
tidy: 
	@rm -f *.o || exit 0
	@rm -f *.i *.hex *.elf *.lss *.eep *.map *.srec *.usersignatures || exit 0
	
packit:
# always good to hard code part of an rm -rf, prevents accidental deletion of whole hard-drive:
	rm -rf $(PACKAGE_DIR)\bistro-HD || exit 0 
	mkdir $(PACKAGE_DIR)\bistro-HD || exit 0
	cp -rf $(PACKAGE_FILES) $(PACKAGE_DIR)\bistro-HD
	$(7ZIP) a -tzip $(PACKAGE_DIR)\bistro-HD.zip $(PACKAGE_DIR)\bistro-HD

#clean does tidy and removes the hex
clean:  tidy
	rm -f *.hex *.elf *.i *.o || exit 0
	rm -f  $(HEX_DIR)/*.hex || exit 0
	rm -f $(BIN_DIR)/*.elf || exit 0
	rm -f $(PREPROCESS_DIR)/*.i || exit 0

sizes: 
	"$(AVR_SIZE)" $(BIN_DIR)/*.elf 

#Atmel studio insists on a clean steap, but you can change the target name of it.
#Make clean isn't always desirable, if for example you're trying to make packit.
#So, a target to do nothing.

nothing:;

.PHONY:tidy sizes $(BATTCHECK_PREPROC) packit nothing
	
