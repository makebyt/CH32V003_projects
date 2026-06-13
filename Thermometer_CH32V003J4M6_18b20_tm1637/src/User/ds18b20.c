#include "ds18b20.h"
#include "main.h"
#include "onewire.h"
#include "ds18b20.h"


static OneWire_t OneWire;

void DS18B20_Init(DS18B20_Resolution_t resolution)
{
    OneWire_Init(&OneWire, DS18B20_GPIO_Port, DS18B20_Pin);
    DS18B20_SetResolution(resolution);
}

void DS18B20_StartAll(void)
{
    OneWire_Reset(&OneWire);
    OneWire_WriteByte(&OneWire, 0xCC); // Skip ROM
    OneWire_WriteByte(&OneWire, 0x44); // Convert T
}

void DS18B20_ReadAll(void)
{
    // §¥§Ý§ñ §à§Õ§ß§à§Ô§à §Õ§Ñ§ä§é§Ú§Ü§Ñ §Þ§à§Ø§ß§à §à§ã§ä§Ñ§Ó§Ú§ä§î §á§å§ã§ä§í§Þ
}

uint8_t DS18B20_GetTemperature(float* temperature)
{
    uint8_t temp_l, temp_h;
    int16_t temp;

    OneWire_Reset(&OneWire);
    OneWire_WriteByte(&OneWire, 0xCC); // Skip ROM
    OneWire_WriteByte(&OneWire, 0xBE); // Read Scratchpad

    temp_l = OneWire_ReadByte(&OneWire);
    temp_h = OneWire_ReadByte(&OneWire);

    temp = (temp_h << 8) | temp_l;
    *temperature = (float)temp / 16.0;

    return 1; // OK
}

void DS18B20_SetResolution(DS18B20_Resolution_t resolution)
{
    OneWire_Reset(&OneWire);
    OneWire_WriteByte(&OneWire, 0xCC); // Skip ROM
    OneWire_WriteByte(&OneWire, 0x4E); // Write Scratchpad

    OneWire_WriteByte(&OneWire, 0);                 // TH
    OneWire_WriteByte(&OneWire, 0);                 // TL
    OneWire_WriteByte(&OneWire, (uint8_t)resolution); // Configuration
}
