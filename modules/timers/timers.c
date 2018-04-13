#include "stm32f4xx_hal.h"
#include "timers/timers.h"
#include <stdbool.h>
#include <stdlib.h>


static volatile uint32_t global_time_ms = 0;
static TIM_HandleTypeDef htim;


struct task
{
	void (*pf)(void);
	struct task *next;
};

struct task *task_list = NULL;



extern void _Error_Handler	(char * file, int line);


void timers_init( void )
{
	/*
	 * 42 000 000 Hz
	 * ( 42 000 000 ) / 2 = 21 000 000 -- [ TIM_CLOCKDIVISION_DIV1 ]
	 * ( 21 000 000 ) / 0x0014 = 1 000 000 -- [ Prescaler - 1 ]
	 * ( 1 000 000 ) / 1 000 = 1 000 -- [ Period ]
	 * tickrate = 1ms
	 */
	__HAL_RCC_TIM2_CLK_ENABLE();
	htim.Instance = TIM2; //42MHz -- APB1
	htim.Init.ClockDivision = TIM_CLOCKDIVISION_DIV2;
	htim.Init.CounterMode= TIM_COUNTERMODE_UP;
	htim.Init.Period = 1000;
	htim.Init.Prescaler = 0x0014;
	if ( HAL_TIM_Base_Init( &htim ) != HAL_OK )
  {
    _Error_Handler( __FILE__, __LINE__ );
  }
	
	__HAL_TIM_ENABLE_IT( &htim, TIM_IT_UPDATE );
	HAL_NVIC_SetPriority( TIM2_IRQn, 15, 15 );
	HAL_NVIC_EnableIRQ( TIM2_IRQn );
	HAL_TIM_Base_Start( &htim );
	
	return;
}


void TIM2_IRQHandler( void )
{
	__HAL_TIM_CLEAR_FLAG( &htim, TIM_FLAG_UPDATE );
	++global_time_ms;
	
	HAL_NVIC_DisableIRQ( TIM2_IRQn );
	if( task_list != NULL )
	{
		struct task *list = task_list;
		while( true )
		{
			list->pf();
			if( list->next == NULL ) break;
			else list = list->next;
		}
	}
	HAL_NVIC_EnableIRQ( TIM2_IRQn );
	return;
}


void delay_ms( uint32_t delay )
{
	volatile uint32_t timeout = global_time_ms + delay + 1;
	HAL_NVIC_SetPriority( TIM2_IRQn, 0, 15 );
	//HAL_NVIC_EnableIRQ( TIM2_IRQn );
	while( timeout >= global_time_ms );
	//HAL_NVIC_DisableIRQ( TIM2_IRQn );
	HAL_NVIC_SetPriority( TIM2_IRQn, 15, 15 );
	return;
}


void add_task( void (*fun_ptr)(void) )
{
	struct task *new_item = ( struct task* )malloc( sizeof( struct task ) );
	new_item->pf = fun_ptr;
	new_item->next = task_list;
	task_list = new_item;
	return;
}
	

