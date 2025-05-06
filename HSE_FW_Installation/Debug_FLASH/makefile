################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include src/subdir.mk
-include hse_config/subdir.mk
-include generate/src/subdir.mk
-include board/subdir.mk
-include RTD/src/subdir.mk
-include Project_Settings/Startup_Code/subdir.mk
-include Project_Settings/Linker_Files/subdir.mk
-include Hse_Files/dcm_register/subdir.mk
-include Hse_Files/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
SECONDARY_FLASH += \
HSE_FW_Installation.bin \

SECONDARY_SIZE += \
HSE_FW_Installation.siz \


# All Target
all: HSE_FW_Installation.elf secondary-outputs

# Tool invocations
HSE_FW_Installation.elf: $(OBJS) C:/Users/150360/NXP_Studio/HSE_FW_Installation/Project_Settings/Linker_Files/linker_flash_s32k344.ld $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: Standard S32DS C Linker'
	arm-none-eabi-gcc -o "HSE_FW_Installation.elf" "@HSE_FW_Installation.args"  $(USER_OBJS)
	@echo 'Finished building target: $@'
	@echo ' '
	$(MAKE) --no-print-directory post-build

HSE_FW_Installation.bin: HSE_FW_Installation.elf
	@echo 'Invoking: Standard S32DS Create Flash Image'
	arm-none-eabi-objcopy -O binary HSE_FW_Installation.elf  "HSE_FW_Installation.bin"
	@echo 'Finished building: $@'
	@echo ' '

HSE_FW_Installation.siz: HSE_FW_Installation.elf
	@echo 'Invoking: Standard S32DS Print Size'
	arm-none-eabi-size --format=berkeley HSE_FW_Installation.elf
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) ./*
	-@echo ' '

post-build:
	-@echo 'Generate secure boot keys and binary files'
	-arm-none-eabi-objcopy -O binary HSE_FW_Installation.elf "HSE_FW_Installation.bin" && "C:/Users/150360/NXP_Studio/HSE_FW_Installation\generate_key.bat"
	-@echo ' '

secondary-outputs: $(SECONDARY_FLASH) $(SECONDARY_SIZE)

.PHONY: all clean dependents post-build

-include ../makefile.targets
