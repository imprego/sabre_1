#include "stm32f4xx_hal.h"
#include "console/console.h"
#include "power/power.h"
#include "indication/indication.h"
#include "mems/mems.h"
#include "shock/shock.h"
#include "timers/timers.h"

#include <stdbool.h>
#include <string.h>


const char* BUILD = "0.5f:13/04/2018";


void _Error_Handler( char* file, int line )
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
	char str_error[ 80 ] = { 0 };
	sprintf( str_error, "eror into: %s at line %d", file, line );
	send( str_error, strlen( str_error ), true );
	abort();
  /* USER CODE END Error_Handler_Debug */
}


void SysTick_Handler(void)
{
  HAL_IncTick();
  HAL_SYSTICK_IRQHandler();
}



void SystemClock_Config( void )
{
	RCC_OscInitTypeDef RCC_OscInitStruct;
  RCC_ClkInitTypeDef RCC_ClkInitStruct;

    /**Configure the main internal regulator output voltage 
    */
	__HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_PWR_CLK_ENABLE();

  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

    /**Initializes the CPU, AHB and APB busses clocks 
    */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 80;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    _Error_Handler( __FILE__, __LINE__ );
  }

    /**Initializes the CPU, AHB and APB busses clocks 
    */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK/*|RCC_CLOCKTYPE_SYSCLK*/
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig( &RCC_ClkInitStruct, FLASH_LATENCY_2 ) != HAL_OK)
  {
    _Error_Handler( __FILE__, __LINE__ );
  }

    /**Configure the Systick interrupt time 
    */
  HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);

    /**Configure the Systick 
    */
  HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

  /* SysTick_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
}





int main( void )
{
	HAL_Init();
	SystemClock_Config();
	
	//interrupts init
	HAL_NVIC_SetPriorityGrouping( NVIC_PRIORITYGROUP_4 );
	
	//delays init
	timers_init();
	
	//power button init
	power_init();
	set_power( POWER_ON );
	
	//uart commands init
	console_init();
	//initial message
	send( "THIS IS ALPHA VERSION SABRE PORJECT",
									strlen("THIS IS ALPHA VERSION SABRE PORJECT"), true );
	
	//leds init
	indication_init();
	set_indication( INDICATION_RED, INDICATION_ON );
	set_indication( INDICATION_GREEN, INDICATION_ON );
	
	//mems sensor init
	mems_init();
	//shock sensor init
	shock_init();
	
	while( true )
	{
		__nop();
	}
}

