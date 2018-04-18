#include "stm32f4xx_hal.h"
#include "timers/timers.h"
#include <stdbool.h>
#include <stdlib.h>


static volatile uint32_t global_time_ms = 0;
static TIM_HandleTypeDef htim;


struct task
{
	unsigned int __execute_rate;
	void (*__pf)(void);
	struct task *__next;
};

struct task *task_list = NULL;



extern void _Error_Handler	(char * file, int line);


void timers_init( void )
{
	/*
	 * 40MHz -- APB1
	 * 80MHz -- TIM2
	 */
	__HAL_RCC_TIM2_CLK_ENABLE();
	htim.Instance = TIM2;
	htim.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	htim.Init.CounterMode= TIM_COUNTERMODE_UP;
	htim.Init.Period = 999;
	htim.Init.Prescaler = 79;
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
	
	//HAL_NVIC_DisableIRQ( TIM2_IRQn );
	if( task_list != NULL )
	{
		struct task *list = task_list;
		while( true )
		{
			if( !( global_time_ms % list->__execute_rate ) )
				list->__pf();
			if( list->__next == NULL ) break;
			else list = list->__next;
		}
	}
	//HAL_NVIC_EnableIRQ( TIM2_IRQn );
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


void add_task( void (*pf)(void), unsigned int execute_rate )
{
	struct task *new_item = ( struct task* )malloc( sizeof( struct task ) );
	new_item->__pf = pf;
	new_item->__next = task_list;
	new_item->__execute_rate = execute_rate;
	task_list = new_item;
	return;
}

uint32_t get_global_time( void )
{
	return global_time_ms;
}
	

