################################################################################
# MRS Version: 2.4.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../lib/ds18b20.c \
../lib/onewire.c \
../lib/tm1637.c 

C_DEPS += \
./lib/ds18b20.d \
./lib/onewire.d \
./lib/tm1637.d 

OBJS += \
./lib/ds18b20.o \
./lib/onewire.o \
./lib/tm1637.o 

DIR_OBJS += \
./lib/*.o \

DIR_DEPS += \
./lib/*.d \

DIR_EXPANDS += \
./lib/*.234r.expand \


# Each subdirectory must supply rules for building sources it contributes
lib/%.o: ../lib/%.c
	@	riscv-none-embed-gcc -march=rv32ecxw -mabi=ilp32e -msmall-data-limit=0 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -I"d:/_GitHub/_projects/Thermometer_CH32V003J4M6_18b20_tm1637/src/Debug" -I"d:/_GitHub/_projects/Thermometer_CH32V003J4M6_18b20_tm1637/src/Core" -I"d:/_GitHub/_projects/Thermometer_CH32V003J4M6_18b20_tm1637/src/User" -I"d:/_GitHub/_projects/Thermometer_CH32V003J4M6_18b20_tm1637/src/Peripheral/inc" -I"d:/_GitHub/_projects/Thermometer_CH32V003J4M6_18b20_tm1637/src/lib" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

