################################################################################
# MRS Version: 2.4.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../User/ch32v00x_it.c \
../User/ds18b20.c \
../User/main.c \
../User/onewire.c \
../User/system_ch32v00x.c \
../User/tm1637.c 

C_DEPS += \
./User/ch32v00x_it.d \
./User/ds18b20.d \
./User/main.d \
./User/onewire.d \
./User/system_ch32v00x.d \
./User/tm1637.d 

OBJS += \
./User/ch32v00x_it.o \
./User/ds18b20.o \
./User/main.o \
./User/onewire.o \
./User/system_ch32v00x.o \
./User/tm1637.o 

DIR_OBJS += \
./User/*.o \

DIR_DEPS += \
./User/*.d \

DIR_EXPANDS += \
./User/*.234r.expand \


# Each subdirectory must supply rules for building sources it contributes
User/%.o: ../User/%.c
	@	riscv-none-embed-gcc -march=rv32ecxw -mabi=ilp32e -msmall-data-limit=0 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -I"d:/_GitHub/_CH32V003_projects/Thermometer_CH32V003J4M6_18b20_tm1637/src/Debug" -I"d:/_GitHub/_CH32V003_projects/Thermometer_CH32V003J4M6_18b20_tm1637/src/Core" -I"d:/_GitHub/_CH32V003_projects/Thermometer_CH32V003J4M6_18b20_tm1637/src/User" -I"d:/_GitHub/_CH32V003_projects/Thermometer_CH32V003J4M6_18b20_tm1637/src/Peripheral/inc" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

