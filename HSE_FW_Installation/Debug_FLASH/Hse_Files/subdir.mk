################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Hse_Files/hse_api.c 

OBJS += \
./Hse_Files/hse_api.o 

C_DEPS += \
./Hse_Files/hse_api.d 


# Each subdirectory must supply rules for building sources it contributes
Hse_Files/%.o: ../Hse_Files/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Standard S32DS C Compiler'
	arm-none-eabi-gcc "@Hse_Files/hse_api.args" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


