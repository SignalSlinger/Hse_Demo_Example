################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/hse_host.c \
../src/hse_host_attrs.c \
../src/main.c 

OBJS += \
./src/hse_host.o \
./src/hse_host_attrs.o \
./src/main.o 

C_DEPS += \
./src/hse_host.d \
./src/hse_host_attrs.d \
./src/main.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Standard S32DS C Compiler'
	arm-none-eabi-gcc "@src/hse_host.args" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


