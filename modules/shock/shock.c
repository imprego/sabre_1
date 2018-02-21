#include "stm32f4xx_hal.h"
#include "shock/shock.h"


static ADC_HandleTypeDef hadc;
static DMA_HandleTypeDef hdma_adc;


extern void _Error_Handler	(char * file, int line);

static uint32_t shocked = 0x00000000;


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
	hadc.Init.ContinuousConvMode = ENABLE;
	hadc.Init.DataAlign = ADC_DATAALIGN_RIGHT;
	hadc.Init.DiscontinuousConvMode = DISABLE;
	hadc.Init.DMAContinuousRequests = ENABLE;
	hadc.Init.EOCSelection = ADC_EOC_SINGLE_CONV;//ADC_EOC_SEQ_CONV;
	hadc.Init.ExternalTrigConv = ADC_SOFTWARE_START;
	hadc.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
	hadc.Init.NbrOfConversion = 1;
	//hadc.Init.NbrOfDiscConversion = ;
	hadc.Init.Resolution = ADC_RESOLUTION_12B;
	hadc.Init.ScanConvMode = DISABLE;
	if ( HAL_ADC_Init( &hadc ) != HAL_OK )
  {
    _Error_Handler( __FILE__, __LINE__ );
  }
	
	ADC_ChannelConfTypeDef adc_channel;
	adc_channel.Channel = ADC_CHANNEL_0;
	adc_channel.Offset = 0;
	adc_channel.Rank = 1;
	adc_channel.SamplingTime = ADC_SAMPLETIME_144CYCLES; //3 15 28 | 56 |  84 112 144 480
	if ( HAL_ADC_ConfigChannel( &hadc, &adc_channel ) != HAL_OK )
  {
    _Error_Handler( __FILE__, __LINE__ );
  }
	
	
	
	__HAL_RCC_DMA2_CLK_ENABLE();
	hdma_adc.Instance = DMA2_Stream0;
  hdma_adc.Init.Channel = DMA_CHANNEL_0;
  hdma_adc.Init.Direction = DMA_PERIPH_TO_MEMORY;
  hdma_adc.Init.PeriphInc = DMA_PINC_DISABLE;
  hdma_adc.Init.MemInc = DMA_MINC_DISABLE;
  hdma_adc.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
  hdma_adc.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
  hdma_adc.Init.Mode = DMA_CIRCULAR;
  hdma_adc.Init.Priority = DMA_PRIORITY_LOW;
  hdma_adc.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
  if ( HAL_DMA_Init( &hdma_adc ) != HAL_OK )
  {
    _Error_Handler( __FILE__, __LINE__ );
  }
	__HAL_LINKDMA( &hadc, DMA_Handle, hdma_adc );
	
	
	if ( HAL_ADC_Start_DMA( &hadc, &shocked, 1 ) != HAL_OK )
  {
    _Error_Handler( __FILE__, __LINE__ );
  }
	
	return;
}



uint16_t get_shock_data( void )
{
	return ( uint16_t )shocked;
}


