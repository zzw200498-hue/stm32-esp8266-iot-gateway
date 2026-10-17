#include "stm32f10x.h"                  // Device header

//extern uint16_t Num;

void Timer_Init(void)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2,ENABLE);
	
	TIM_InternalClockConfig(TIM2);
	
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	TIM_TimeBaseInitStructure.TIM_ClockDivision=TIM_CKD_DIV1;
	TIM_TimeBaseInitStructure.TIM_CounterMode=TIM_CounterMode_Up;
	TIM_TimeBaseInitStructure.TIM_Period= 10000-1;  //   72k/7200/10000 
	TIM_TimeBaseInitStructure.TIM_Prescaler= 7200-1;//   72k/7200
	TIM_TimeBaseInitStructure.TIM_RepetitionCounter=0 ;
	TIM_TimeBaseInit(TIM2,&TIM_TimeBaseInitStructure);
	
	TIM_ClearFlag(TIM2,TIM_IT_Update);
	TIM_ITConfig(TIM2,TIM_IT_Update,ENABLE);
	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	
	NVIC_InitTypeDef NVIC_InitSturcture;
	NVIC_InitSturcture.NVIC_IRQChannel=TIM2_IRQn;
	NVIC_InitSturcture.NVIC_IRQChannelCmd=ENABLE;
	NVIC_InitSturcture.NVIC_IRQChannelPreemptionPriority=2;
	NVIC_InitSturcture.NVIC_IRQChannelSubPriority=1;
	NVIC_Init(&NVIC_InitSturcture);
	
	TIM_Cmd(TIM2,ENABLE);
}

/*
void TIM2_IRQHandler(void)
{
	if(TIM_GetITStatus(TIM2,TIM_IT_Update)==SET)
	{
		Num++;
		TIM_ClearITPendingBit(TIM2,TIM_IT_Update);
	}
}
*/
