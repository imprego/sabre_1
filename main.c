#include "stm32f4xx_hal.h"

#include "console/console.h"
#include "power/power.h"
#include "indication/indication.h"
#include "mems/mems.h"
#include "shock/shock.h"

#include "FreeRTOS.h"
#include "task.h"

#include <stdbool.h>
#include <string.h>


const char* BUILD = "0.7c:08/05/2018";

void _Error_Handler( char* file, int line );
static void SystemClock_Config( void );
void vSystemSynk( void *vParams );


int main( void )
{
	//leds init
	indication_init();
	
	
	//power_init();
	set_power( POWER_ON );
	
	
	//clocks config
	SystemClock_Config();
	
	
	//interrupts init
	HAL_NVIC_SetPriorityGrouping( NVIC_PRIORITYGROUP_4 );
	
	
	//uart commands init
	if( xTaskCreate( vConsoleInitProc,
									"ConsoleInit",
									configMINIMAL_STACK_SIZE,
									NULL,
									tskIDLE_PRIORITY + 4,
									NULL ) != pdPASS ) _Error_Handler( __FILE__, __LINE__ );
	extern TaskHandle_t vConsoleWorkProcHandle;
	if( xTaskCreate( vConsoleWorkProc,
									"ConsoleWork",
									configMINIMAL_STACK_SIZE * 4,
									NULL,
									tskIDLE_PRIORITY + 1,
									&vConsoleWorkProcHandle ) != pdPASS ) _Error_Handler( __FILE__, __LINE__ );
	
	
	//mems sensor init
	if( xTaskCreate( vMemsInitProc,
									"MemsInit",
									configMINIMAL_STACK_SIZE,
									NULL,
									tskIDLE_PRIORITY + 4,
									NULL ) != pdPASS ) _Error_Handler( __FILE__, __LINE__ );
	if( xTaskCreate( vMemsWorkProc,
									"MemsWork",
									configMINIMAL_STACK_SIZE * 2,
									NULL,
									tskIDLE_PRIORITY + 1,
									NULL ) != pdPASS ) _Error_Handler( __FILE__, __LINE__ );
	if( xTaskCreate( vMemsSyncProc,
									"MemsSync",
									configMINIMAL_STACK_SIZE,
									NULL,
									tskIDLE_PRIORITY + 2,
									NULL ) != pdPASS ) _Error_Handler( __FILE__, __LINE__ );
	
	
	//shock sensor init
	shock_init();
	
	
	//flush buffers
	if( xTaskCreate( vSystemSynk,
									"SystemSynk",
									configMINIMAL_STACK_SIZE,
									NULL,
									tskIDLE_PRIORITY + 1,
									NULL ) != pdPASS ) _Error_Handler( __FILE__, __LINE__ );
	
	
	//start sheduler
	vTaskStartScheduler();
	
	
	while( true )
	{
		__nop();
	}
}





static void SystemClock_Config( void )
{
	RCC_OscInitTypeDef RCC_OscInitStruct;
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
	
	__HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_PWR_CLK_ENABLE();

  __HAL_PWR_VOLTAGESCALING_CONFIG( PWR_REGULATOR_VOLTAGE_SCALE2 );
	
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
	
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig( &RCC_ClkInitStruct, FLASH_LATENCY_2 ) != HAL_OK)
  {
    _Error_Handler( __FILE__, __LINE__ );
  }
}


void vSystemSynk( void* vParams )
{
	//flush buffers
	flush_shock_buf();
	flush_fifo_buf();
	
	//and destroy
	vTaskDelete( NULL );
	vTaskDelay( 0 );
	return;
}


HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority)
{
	return HAL_OK;
}





void _Error_Handler( char* file, int line )
{
	volatile char* __file = file;
	volatile int __line = line;
	set_indication( INDICATION_LED_GREEN, INDICATION_STATE_ON );
	while( true );
}

void HardFault_Handler( char* file, int line )
{
	set_indication( INDICATION_LED_RED, INDICATION_STATE_ON );
	while( true );
}

void vApplicationStackOverflowHook( void )
{
	_Error_Handler( __FILE__, __LINE__ );
	return;
}

void vApplicationMallocFailedHook( void )
{
	_Error_Handler( __FILE__, __LINE__ );
	return;
}

