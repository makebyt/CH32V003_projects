################################################################################
# MRS Version: 2.3.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../libraries/ds18b20.c \
../libraries/onewire.c 

C_DEPS += \
./libraries/ds18b20.d \
./libraries/onewire.d 

OBJS += \
./libraries/ds18b20.o \
./libraries/onewire.o 

DIR_OBJS += \
./libraries/*.o \

DIR_DEPS += \
./libraries/*.d \

DIR_EXPANDS += \
./libraries/*.234r.expand \


# Each subdirectory must supply rules for building sources it contributes
libraries/%.o: ../libraries/%.c
	@	riscv-none-embed-gcc -march=rv32ecxw -mabi=ilp32e -msmall-data-limit=0 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -I"d:/Download/CH32V003J4M6_00/Debug" -I"d:/Download/CH32V003J4M6_00/Core" -I"d:/Download/CH32V003J4M6_00/User" -I"d:/Download/CH32V003J4M6_00/Peripheral/inc" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

