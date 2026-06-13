################################################################################
# MRS Version: 2.4.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/core_riscv.c 

C_DEPS += \
./Core/core_riscv.d 

OBJS += \
./Core/core_riscv.o 

DIR_OBJS += \
./Core/*.o \

DIR_DEPS += \
./Core/*.d \

DIR_EXPANDS += \
./Core/*.234r.expand \


# Each subdirectory must supply rules for building sources it contributes
Core/%.o: ../Core/%.c
	@	riscv-none-embed-gcc -march=rv32ecxw -mabi=ilp32e -msmall-data-limit=0 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -I"d:/_GitHub/_CH32V003_projects/Thermometer_CH32V003J4M6_18b20_tm1637/src/Debug" -I"d:/_GitHub/_CH32V003_projects/Thermometer_CH32V003J4M6_18b20_tm1637/src/Core" -I"d:/_GitHub/_CH32V003_projects/Thermometer_CH32V003J4M6_18b20_tm1637/src/User" -I"d:/_GitHub/_CH32V003_projects/Thermometer_CH32V003J4M6_18b20_tm1637/src/Peripheral/inc" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

