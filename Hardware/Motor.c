#include "stm32f10x.h"                  // Device header
#include "Motor.h"
#include "PWM.h"

/*
 * 函    数：直流电机初始化
 * 参    数：无
 * 返 回 值：无
 * 说    明：PA4/PA5 控制方向（接驱动芯片的 AIN1/AIN2），PA0 输出 PWM 调速
 */
void Motor_Init(void)
{
	/*开启时钟*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);		//开启GPIOA的时钟

	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);						//PA4、PA5 推挽输出

	/*上电默认停转：方向脚一高一低但 PWM 占空比为 0*/
	GPIO_SetBits(GPIOA, GPIO_Pin_4);
	GPIO_ResetBits(GPIOA, GPIO_Pin_5);

	PWM_Init();													//初始化 PWM 底层
}

/*
 * 函    数：直流电机设置速度
 * 参    数：Speed 要设置的速度，范围：-100~100（负数为反转，风扇只用正转即可）
 * 返 回 值：无
 */
void Motor_SetSpeed(int8_t Speed)
{
	if (Speed >= 0)							//正转
	{
		GPIO_SetBits(GPIOA, GPIO_Pin_4);	//PA4置高电平
		GPIO_ResetBits(GPIOA, GPIO_Pin_5);	//PA5置低电平，方向为正转
		PWM_SetCompare1(Speed);				//PWM 占空比 = 速度值
	}
	else									//反转
	{
		GPIO_ResetBits(GPIOA, GPIO_Pin_4);	//PA4置低电平
		GPIO_SetBits(GPIOA, GPIO_Pin_5);	//PA5置高电平，方向为反转
		PWM_SetCompare1(-Speed);			//取绝对值，PWM 只收正数
	}
}
