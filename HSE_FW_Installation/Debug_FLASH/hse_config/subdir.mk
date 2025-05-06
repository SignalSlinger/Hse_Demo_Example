################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../hse_config/hse_config.c 

OBJS += \
./hse_config/hse_config.o 

C_DEPS += \
./hse_config/hse_config.d 


# Each subdirectory must supply rules for building sources it contributes
hse_config/%.o: ../hse_config/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Standard S32DS C Compiler'
	arm-none-eabi-gcc "@hse_config/hse_config.args" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


