// DS18B20 1-Wire temperature reader for WCH CH32V003

#include "main.h"
#include "debug.h"
#include "onewire.h"
#include "ds18b20.h"
#include "string.h"
#include "tm1637.h"

float temperature;
char message[64];
uint8_t j = 0;

void GPIO_Toggle_INIT(void)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

    // Конфигурируем DS18B20_Pin как open-drain output
    GPIO_InitStructure.GPIO_Pin = DS18B20_Pin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(DS18B20_GPIO_Port, &GPIO_InitStructure);

    // Конфигурируем LED_Pin как обычный push-pull output
    GPIO_InitStructure.GPIO_Pin = LED_Pin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(LED_GPIO_Port, &GPIO_InitStructure);
}


int main(void)
{
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
  Delay_Init();
  USART_Printf_Init(115200);

  GPIO_Toggle_INIT();

    TM1637_Init();
    TM1637_SetBrightness(3);    // яркость 0-7
    uint8_t LED[4] = {1,2,3,4}; // задаем значение дисплея
    uint8_t dp = 1;             // 1-точка есть
    uint8_t minus = 0;          // знак минуса
    uint16_t val;

  DS18B20_Init(DS18B20_Resolution_12bits);
  printf("Started!\r\n");
  
  while (1)
  {
	  DS18B20_ReadAll();
	  GPIO_WriteBit(TEST_GPIO_Port, TEST_Pin, 1);
    DS18B20_StartAll();
    GPIO_WriteBit(TEST_GPIO_Port, TEST_Pin, 0);
		

    float temperature;

    if(DS18B20_GetTemperature(&temperature))
    {
        //DS18B20_GetROM(0, ROM_tmp);

        // temperature=-0.93; //для Теста

        if(temperature < 0)
          {
          temperature = -temperature;
          minus=1;
          printf("Temp: -%d.%02d oC\n\r",  (int)temperature, (int)(temperature*100) % 100);
          }
        else
          {
          minus=0;
          printf("Temp: %d.%02d oC\n\r",  (int)temperature, (int)(temperature*100) % 100);
          }
    }

		Delay_Ms(1000);

    // LED Blink
    GPIO_WriteBit(LED_GPIO_Port, LED_Pin, (j == 0) ? (j = Bit_SET) : (j = Bit_RESET));

    // TM1637 дисплей, разбиваем на разряды
    val=temperature*100+5;
    LED[0] = val / 1000;
    LED[1] = (val / 100) % 10;
    LED[2] = (val / 10) % 10;
    //LED[3] = val % 10;
    if (LED[0]==0) LED[0] = 10; //гасим первый 0

    // Выводим знак в LED[3]
    if(minus)
      LED[3] = 11; // Выводим знак минус
    else
      LED[3] = 14; // Выводим знак градуса

    // Выводим на дисплей
    for(int i = 0; i < 4; i++)
      {
      TM1637_DisplayDigit(i, LED[i], (i == 1 && dp));
      } 

  }
}
 