#include "stm32f4xx_hal.h"
#include "shock/shock.h"


ADC_HandleTypeDef hadc;


extern void _Error_Handler	(char * file, int line);

static uint16_t shocked = 0x0000;


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
	hadc.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2;
	hadc.Init.ContinuousConvMode = ENABLE;
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
	if (HAL_ADC_Init( &hadc ) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }
	
	ADC_ChannelConfTypeDef adc_channel;
	adc_channel.Channel = ADC_CHANNEL_0;
	adc_channel.Offset = 0;
	adc_channel.Rank = 1;
	adc_channel.SamplingTime = ADC_SAMPLETIME_144CYCLES; //3 15 28 | 56 |  84 112 144 480
	HAL_ADC_ConfigChannel( &hadc, &adc_channel );
	
	__HAL_ADC_ENABLE_IT( &hadc, ADC_IT_EOC );
	__HAL_ADC_CLEAR_FLAG( &hadc, ADC_FLAG_EOC );
	HAL_NVIC_SetPriority( ADC_IRQn, 0, 1 );
  HAL_NVIC_EnableIRQ( ADC_IRQn );
	
	HAL_ADC_Start_IT( &hadc );
	
	return;
}



uint16_t get_shock_data( void )
{
	return shocked;
}



void ADC_IRQHandler( void )
{
	__HAL_ADC_CLEAR_FLAG( &hadc, ADC_FLAG_EOC );
	
	uint16_t temp = ( uint16_t )HAL_ADC_GetValue( &hadc );
	
	if( shocked < temp ) shocked = temp;
}


