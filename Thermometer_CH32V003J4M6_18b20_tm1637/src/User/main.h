#ifndef __MAIN_H
#define __MAIN_H

#define TEST_Pin GPIO_Pin_2
#define TEST_GPIO_Port GPIOA
#define DS18B20_Pin GPIO_Pin_2
#define DS18B20_GPIO_Port GPIOA
#define LED_Pin GPIO_Pin_4
#define LED_GPIO_Port GPIOC

void _Error_Handler(char *, int);

#define Error_Handler() _Error_Handler(__FILE__, __LINE__)

#endif
