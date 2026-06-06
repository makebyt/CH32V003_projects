#include "tm1637.h"
#include "ch32v00x_gpio.h"
#include "debug.h"

#define TM1637_CLK_PORT GPIOC
#define TM1637_DIO_PORT GPIOC
#define TM1637_CLK_PIN  GPIO_Pin_1
#define TM1637_DIO_PIN  GPIO_Pin_2

// Массив сегментов 0-9 + A-F + символы
static const uint8_t segs[25] = {
    0x3f, // 0
    0x06, // 1
    0x5b, // 2
    0x4f, // 3
    0x66, // 4
    0x6d, // 5
    0x7d, // 6
    0x07, // 7
    0x7f, // 8
    0x6f, // 9
    0x00, //[10]   пусто
    0x40, //[11] - минус
    0x01, //[12] ` верхняя черта
    0x08, //[13] _ нижняя черта
    0x63, //[14] * знак градуса
    0x1c, //[15] u-маленткая
    0x03, //[16] ``верхний правый угол
    0x0c, //[17] __нижний правый угол
    0x58, //[18] c-маленткая
	//0x39, //[18] C-большая
	0x5e, //(19) d-маленткая
    0x79, //[20] E-большая
    0x71, //[21] F-большая
    0x76, //[22] H-большая
    0x38, //[23] L-большая
    0x73  //[24] P-большая
};

// Пауза для длительности сигнала. при 50 пауза = 6-8мкС. Весь пакет = 2мС
static void TM1637_Delay(void) 
{
    for(volatile int i=0; i<50; i++);
}

void TM1637_Start(void)
{
    GPIO_WriteBit(TM1637_DIO_PORT, TM1637_DIO_PIN, Bit_SET);
    GPIO_WriteBit(TM1637_CLK_PORT, TM1637_CLK_PIN, Bit_SET);
    TM1637_Delay();
    GPIO_WriteBit(TM1637_DIO_PORT, TM1637_DIO_PIN, Bit_RESET);
}

void TM1637_Stop(void)
{
    GPIO_WriteBit(TM1637_CLK_PORT, TM1637_CLK_PIN, Bit_RESET);
    TM1637_Delay();
    GPIO_WriteBit(TM1637_DIO_PORT, TM1637_DIO_PIN, Bit_RESET);
    TM1637_Delay();
    GPIO_WriteBit(TM1637_CLK_PORT, TM1637_CLK_PIN, Bit_SET);
    TM1637_Delay();
    GPIO_WriteBit(TM1637_DIO_PORT, TM1637_DIO_PIN, Bit_SET);
}

void TM1637_WriteByte(uint8_t b)
{
    for(uint8_t i=0;i<8;i++)
    {
        GPIO_WriteBit(TM1637_CLK_PORT, TM1637_CLK_PIN, Bit_RESET);
        GPIO_WriteBit(TM1637_DIO_PORT, TM1637_DIO_PIN, (b & 0x01) ? Bit_SET : Bit_RESET);
        TM1637_Delay();
        GPIO_WriteBit(TM1637_CLK_PORT, TM1637_CLK_PIN, Bit_SET);
        TM1637_Delay();
        b >>= 1;
    }

    // ACK
    GPIO_WriteBit(TM1637_CLK_PORT, TM1637_CLK_PIN, Bit_RESET);
    TM1637_Delay();
    GPIO_WriteBit(TM1637_DIO_PORT, TM1637_DIO_PIN, Bit_SET); // INPUT
    TM1637_Delay();
    GPIO_WriteBit(TM1637_CLK_PORT, TM1637_CLK_PIN, Bit_SET);
    TM1637_Delay();
}

void TM1637_Init(void)
{
    GPIO_InitTypeDef s = {0};
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

    s.GPIO_Pin = TM1637_CLK_PIN | TM1637_DIO_PIN;
    s.GPIO_Mode = GPIO_Mode_Out_OD;
    s.GPIO_Speed = GPIO_Speed_10MHz;
    GPIO_Init(GPIOC, &s);
}

void TM1637_SetBrightness(uint8_t level)
{
    TM1637_Start();
    TM1637_WriteByte(0x88 | (level & 7));
    TM1637_Stop();
}

void TM1637_DisplayOff(void)
{
    TM1637_Start();
    TM1637_WriteByte(0x80);     // OFF
    TM1637_Stop();
}

void TM1637_DisplayDigit(uint8_t pos, uint8_t value, uint8_t dot)
{
    uint8_t seg = (value < 30) ? segs[value] : 0x00;

    if(dot) seg |= 0x80;  // ← точка

    TM1637_Start();
    TM1637_WriteByte(0x44);      // фиксированная адресация
    TM1637_Stop();

    TM1637_Start();
    TM1637_WriteByte(0xC0 + pos);
    TM1637_WriteByte(seg);
    TM1637_Stop();
}
