#include "stm32f4xx_hal.h"
#include "shock/shock.h"


ADC_HandleTypeDef hadc;


extern void _Error_Handler	(char * file, int line);


void shock_init( void )
{
	__HAL_RCC_GPIOA_CLK_ENABLE();
	GPIO_InitTypeDef gpio;
	//gpio.Alternate = GPIO_AF5_SPI2;
	gpio.Mode = GPIO_MODE_ANALOG;
	gpio.Pin = GPIO_PIN_0;
	gpio.Pull = GPIO_NOPULL;
	gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	HAL_GPIO_Init( GPIOA, &gpio );
	
	
	__HAL_RCC_ADC1_CLK_ENABLE();
	hadc.Instance = ADC1;
	hadc.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
	hadc.Init.ContinuousConvMode = DISABLE;
	hadc.Init.DataAlign = ADC_DATAALIGN_RIGHT;
	hadc.Init.DiscontinuousConvMode = DISABLE;
	hadc.Init.DMAContinuousRequests = DISABLE;
	hadc.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
	hadc.Init.ExternalTrigConv = ADC_SOFTWARE_START;
	hadc.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
	hadc.Init.NbrOfConversion = 1;
	//hadc.Init.NbrOfDiscConversion = ;
	hadc.Init.Resolution = ADC_RESOLUTION_12B;
	hadc.Init.ScanConvMode = DISABLE;
	if (HAL_ADC_Init(&hadc) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }
	
	return;
}



uint16_t get_shock_data( void )
{
	uint16_t shocked = 0;
	
	HAL_ADC_Start( &hadc );
	
	//HAL_ADC_PollForConversion( hadc, 10 );
	
	shocked = ( uint16_t )HAL_ADC_GetValue( &hadc );
	
	HAL_ADC_Stop( &hadc );
	
	return shocked;
}


