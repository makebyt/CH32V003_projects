#ifndef __DS18B20_H
#define __DS18B20_H

#include "onewire.h"

typedef enum {
    DS18B20_Resolution_9bits  = 0x1F,
    DS18B20_Resolution_10bits = 0x3F,
    DS18B20_Resolution_11bits = 0x5F,
    DS18B20_Resolution_12bits = 0x7F
} DS18B20_Resolution_t;

void DS18B20_Init(DS18B20_Resolution_t resolution);
void DS18B20_StartAll(void);
void DS18B20_ReadAll(void);
uint8_t DS18B20_GetTemperature(float* temperature);
void DS18B20_SetResolution(DS18B20_Resolution_t resolution);

#endif
