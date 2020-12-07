################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
ifneq ($(strip $(ASM_DEPS)),)
-include $(ASM_DEPS)
endif
ifneq ($(strip $(S_DEPS)),)
-include $(S_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
LSS += \
BLF-VLD.lss \

FLASH_IMAGE += \
BLF-VLD.hex \

EEPROM_IMAGE += \
BLF-VLD.eep \

SIZEDUMMY += \
sizedummy \


# All Target
all: BLF-VLD.elf secondary-outputs

# Tool invocations
BLF-VLD.elf: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: AVR C Linker'
	avr-gcc -Wl,-Map,BLF-VLD.map -mmcu=attiny13 -o"BLF-VLD.elf" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

BLF-VLD.lss: BLF-VLD.elf
	@echo 'Invoking: AVR Create Extended Listing'
	-avr-objdump -h -S BLF-VLD.elf  >"BLF-VLD.lss"
	@echo 'Finished building: $@'
	@echo ' '

BLF-VLD.hex: BLF-VLD.elf
	@echo 'Create Flash image (ihex format)'
	-avr-objcopy -R .eeprom -O ihex BLF-VLD.elf  "BLF-VLD.hex"
	@echo 'Finished building: $@'
	@echo ' '

BLF-VLD.eep: BLF-VLD.elf
	@echo 'Create eeprom image (ihex format)'
	-avr-objcopy -j .eeprom --no-change-warnings --change-section-lma .eeprom=0 -O ihex BLF-VLD.elf  "BLF-VLD.eep"
	@echo 'Finished building: $@'
	@echo ' '

sizedummy: BLF-VLD.elf
	@echo 'Invoking: Print Size'
	-avr-size --format=avr --mcu=attiny13 BLF-VLD.elf
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(OBJS)$(C_DEPS)$(ASM_DEPS)$(EEPROM_IMAGE)$(FLASH_IMAGE)$(ELFS)$(LSS)$(S_DEPS)$(SIZEDUMMY)$(S_UPPER_DEPS) BLF-VLD.elf
	-@echo ' '

secondary-outputs: $(LSS) $(FLASH_IMAGE) $(EEPROM_IMAGE) $(SIZEDUMMY)

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets
