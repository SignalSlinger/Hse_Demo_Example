################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Hse_Files/dcm_register/hse_dcm_register.c 

OBJS += \
./Hse_Files/dcm_register/hse_dcm_register.o 

C_DEPS += \
./Hse_Files/dcm_register/hse_dcm_register.d 


# Each subdirectory must supply rules for building sources it contributes
Hse_Files/dcm_register/%.o: ../Hse_Files/dcm_register/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Standard S32DS C Compiler'
	arm-none-eabi-gcc "@Hse_Files/dcm_register/hse_dcm_register.args" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


