#ifndef __DHT11_H
#define __DHT11_H

#include "stm32f10x.h"

/*
 * DHT11 温湿度传感器驱动（单总线协议）
 * 数据引脚可通过下面三个宏更换，默认 PA1
 */
#define DHT11_PORT  GPIOA
#define DHT11_PIN   GPIO_Pin_1
#define DHT11_RCC   RCC_APB2Periph_GPIOA

/*
 * 初始化数据引脚（时钟使能 + 空闲态输出高电平）。
 * 返回：0 成功
 */
uint8_t DHT11_Init(void);

/*
 * 读取一次温湿度。
 * 参数：humi_int  湿度整数部分（如 65 表示 65%）
 *       humi_dec  湿度小数部分（DHT11 恒为 0）
 *       temp_int  温度整数部分（如 25 表示 25℃）
 *       temp_dec  温度小数部分（0~9，表示 0.x ℃）
 * 返回：0 成功，1 传感器无应答，2 校验和错误
 *
 * 注意：两次读取间隔必须大于 1 秒，否则传感器不响应
 */
uint8_t DHT11_ReadData(uint8_t *humi_int, uint8_t *humi_dec,
                       uint8_t *temp_int, uint8_t *temp_dec);

#endif
