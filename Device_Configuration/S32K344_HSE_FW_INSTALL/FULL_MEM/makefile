################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include src/mu/subdir.mk
-include src/Flash/subdir.mk
-include src/subdir.mk
-include Project_Settings/Startup_Code/subdir.mk
-include Project_Settings/Linker_Files/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
SECONDARY_SIZE += \
S32K344_HSE_FW_INSTALL.siz \


# All Target
all: S32K344_HSE_FW_INSTALL.elf secondary-outputs

# Tool invocations
S32K344_HSE_FW_INSTALL.elf: $(OBJS) C:\NXP\S32K3_HSE_DemoExamples_1_0_0\S32K3_HSE_DemoExamples\Device_Configuration\S32K344_HSE_FW_INSTALL\Project_Settings\Linker_Files\S32K344_flash_full_mem.ld $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: Standard S32DS C Linker'
	arm-none-eabi-gcc -o "S32K344_HSE_FW_INSTALL.elf" "@S32K344_HSE_FW_INSTALL.args"  $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

S32K344_HSE_FW_INSTALL.siz: S32K344_HSE_FW_INSTALL.elf
	@echo 'Invoking: Standard S32DS Print Size'
	arm-none-eabi-size --format=berkeley S32K344_HSE_FW_INSTALL.elf
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) ./*
	-@echo ' '

secondary-outputs: $(SECONDARY_SIZE)

.PHONY: all clean dependents

-include ../makefile.targets
