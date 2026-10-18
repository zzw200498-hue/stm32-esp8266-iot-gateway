#include "stm32f10x.h"                  // Device header
#include "PWM.h"

/*
 * PWM 初始化：TIM2 通道 1，输出引脚 PA0
 * PWM 频率 = 72MHz / (PSC*ARR) = 72MHz / (36*100) = 20kHz（超出人耳范围，电机无啸叫）
 * 占空比 = CCR / 100，所以 CCR 直接当"速度百分比"用
 */
void PWM_Init(void)
{
	/*开启时钟*/
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);			//开启TIM2的时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);			//开启GPIOA的时钟

	/*GPIO初始化：PA0 复用推挽，交给 TIM2 通道 1 控制*/
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/*配置时钟源*/
	TIM_InternalClockConfig(TIM2);		//选择TIM2为内部时钟

	/*时基单元初始化*/
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;     //时钟分频，不分频
	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up; //向上计数
	TIM_TimeBaseInitStructure.TIM_Period = 100 - 1;                 //ARR=99，CCR 范围 0~100
	TIM_TimeBaseInitStructure.TIM_Prescaler = 36 - 1;               //PSC=35，72M/36=2MHz
	TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;            //高级定时器才用
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseInitStructure);

	/*输出比较初始化：通道 1，PWM 模式 1*/
	TIM_OCInitTypeDef TIM_OCInitStructure;
	TIM_OCStructInit(&TIM_OCInitStructure);                         //先填默认值
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;               //PWM模式1
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;       //高极性
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;   //输出使能
	TIM_OCInitStructure.TIM_Pulse = 0;								//初始占空比 0（电机停转）
	TIM_OC1Init(TIM2, &TIM_OCInitStructure);                        //TIM2 通道 1

	/*TIM使能*/
	TIM_Cmd(TIM2, ENABLE);
}

/*
 * 函    数：设置 PWM 占空比
 * 参    数：Compare 要写入的 CCR 值，范围 0~100
 * 返 回 值：无
 * 说    明：占空比 = CCR / 100，所以传 50 就是 50% 占空比
 */
void PWM_SetCompare1(uint16_t Compare)
{
	TIM_SetCompare1(TIM2, Compare);
}
