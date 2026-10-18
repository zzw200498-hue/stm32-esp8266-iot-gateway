#include "stm32f10x.h"
#include "DHT11.h"
#include "Delay.h"

/*
 * 引脚读写快捷宏：
 * 置高 / 置低用于主机发起始信号；读引脚用于采样 DHT11 的应答和数据位
 */
#define DHT11_PIN_HIGH()  GPIO_SetBits(DHT11_PORT, DHT11_PIN)
#define DHT11_PIN_LOW()   GPIO_ResetBits(DHT11_PORT, DHT11_PIN)
#define DHT11_PIN_READ()  GPIO_ReadInputDataBit(DHT11_PORT, DHT11_PIN)

/*
 * 把数据引脚切换为推挽输出（主机掌握总线，发起始信号时用）
 */
static void DHT11_Mode_Output(void)
{
    GPIO_InitTypeDef gpio_struct;

    gpio_struct.GPIO_Pin   = DHT11_PIN;
    gpio_struct.GPIO_Mode  = GPIO_Mode_Out_PP;
    gpio_struct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(DHT11_PORT, &gpio_struct);
}

/*
 * 把数据引脚切换为上拉输入（释放总线，等 DHT11 说话）
 * 上拉输入保证总线空闲时稳定为高电平
 */
static void DHT11_Mode_Input(void)
{
    GPIO_InitTypeDef gpio_struct;

    gpio_struct.GPIO_Pin  = DHT11_PIN;
    gpio_struct.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(DHT11_PORT, &gpio_struct);
}

/*
 * 初始化：使能 GPIO 时钟，总线置为空闲高电平。
 * 返回：0 成功
 */
uint8_t DHT11_Init(void)
{
    GPIO_InitTypeDef gpio_struct;

    RCC_APB2PeriphClockCmd(DHT11_RCC, ENABLE);

    gpio_struct.GPIO_Pin   = DHT11_PIN;
    gpio_struct.GPIO_Mode  = GPIO_Mode_Out_PP;
    gpio_struct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(DHT11_PORT, &gpio_struct);

    DHT11_PIN_HIGH();       /* 单总线空闲态为高电平 */

    return 0;
}

/*
 * 发送起始信号：主机拉低 20ms → 释放总线 → 切输入等 DHT11 应答。
 * DHT11 收到后会主动拉低 80us 再拉高 80us 作为应答。
 */
static void DHT11_Start(void)
{
    DHT11_Mode_Output();
    DHT11_PIN_LOW();
    Delay_ms(20);           /* datasheet 要求拉低至少 18ms，取 20ms 留余量 */
    DHT11_PIN_HIGH();       /* 释放总线 */
    Delay_us(30);           /* 主机释放 20~40us 后交出总线控制权 */
    DHT11_Mode_Input();     /* 切输入，之后由 DHT11 驱动总线 */
}

static uint8_t DHT11_ReadBit(void)
{
    uint8_t bit_value = 0;
    uint16_t timeout = 0;
    uint16_t count = 0;
    while (DHT11_PIN_READ() == 0) {
        if(++timeout > 100) { /* 超时保护，约 100us */
            return 0; /* 传感器不在 */
		}
        Delay_us(1); /* 延时 1us，避免计数过快 */
    }
    while (DHT11_PIN_READ() == 1) {
        count++; /* 计数高电平持续时间 */
        Delay_us(1); /* 延时 1us，避免计数过快 */
		if(count>100) { /* 超时保护，约 1000us */
            break;
		}
    } 
    if (count > 15) {
        bit_value = 1;
    }
    else {
        bit_value = 0;
    }

    return bit_value;
}

static uint8_t DHT11_ReadByte(void)
{
    uint8_t i;
    uint8_t byte_value = 0;
    for(i=0;i<8;i++){
        byte_value<<=1;
        byte_value|=DHT11_ReadBit();
    }
    return byte_value;
}

uint8_t DHT11_ReadData(uint8_t *humi_int, uint8_t *humi_dec,
                       uint8_t *temp_int, uint8_t *temp_dec)
{
    uint8_t buf[5];
    uint16_t timeout = 0;
    uint8_t i;
    uint8_t check_sum;
    DHT11_Start();

    timeout=0;
    while (DHT11_PIN_READ()==1)
    {
        if(++timeout>100){
            return 1;
        }
        Delay_us(1);
    }

    timeout=0;
    while (DHT11_PIN_READ()==0)
    {
        if(++timeout>100){
            return 1;
        }
        Delay_us(1);
    }
    
    timeout=0;
    while (DHT11_PIN_READ()==1)
    {
        if(++timeout>100){
            return 1;
        }
        Delay_us(1);
    }

    for(i=0;i<5;i++){
        buf[i]=DHT11_ReadByte();
    }

    check_sum = buf[0] + buf[1] + buf[2] + buf[3];
    if (check_sum != buf[4]) {
        return 2; // 校验失败
    }

    // 4. 校验通过，写入指针
    *humi_int = buf[0];
    *humi_dec = buf[1];
    *temp_int = buf[2];
    *temp_dec = buf[3];

    return 0;
}
