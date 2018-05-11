#include "stm32f4xx_hal.h"
#include "shock/shock.h"

#include <string.h>

#define SHOCK_BUF_SIZE						25
#define SHOCK_BUF_SECTIONS				4

extern void _Error_Handler( char* file, int line );


static ADC_HandleTypeDef hadc;
static DMA_HandleTypeDef hdma_adc;
static TIM_HandleTypeDef htim;

static uint16_t shock_buf[ SHOCK_BUF_SECTIONS ][ SHOCK_BUF_SIZE ] = { 0 };
static uint16_t *shock_buf_ptr[ SHOCK_BUF_SECTIONS ] = { NULL };


void shock_init( void )
{
	__HAL_RCC_GPIOA_CLK_ENABLE();
	GPIO_InitTypeDef gpio;
	gpio.Mode = GPIO_MODE_ANALOG;
	gpio.Pin = GPIO_PIN_0;
	gpio.Pull = GPIO_NOPULL;
	gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	HAL_GPIO_Init( GPIOA, &gpio );
	
	__HAL_RCC_ADC1_CLK_ENABLE();
	hadc.Instance = ADC1;
	// 80/6=13.3MHz < 15(typ) or 18(max) up to 2.4V
	// 80/4=20MHz < 30(typ) or 36(max) up to 3.6V
	hadc.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
	hadc.Init.ContinuousConvMode = DISABLE;
	hadc.Init.DataAlign = ADC_DATAALIGN_RIGHT;
	hadc.Init.DiscontinuousConvMode = ENABLE;
	hadc.Init.DMAContinuousRequests = ENABLE;
	hadc.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
	hadc.Init.ExternalTrigConv = ADC_EXTERNALTRIGCONV_T2_TRGO;
	hadc.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_RISING;
	hadc.Init.NbrOfConversion = 1;
	hadc.Init.NbrOfDiscConversion = 1;
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
	adc_channel.SamplingTime = ADC_SAMPLETIME_84CYCLES; //3 15 28 | 56 |  84 112 144 480
	if ( HAL_ADC_ConfigChannel( &hadc, &adc_channel ) != HAL_OK )
  {
    _Error_Handler( __FILE__, __LINE__ );
  }
	
	__HAL_RCC_DMA2_CLK_ENABLE();
	hdma_adc.Instance = DMA2_Stream0;
  hdma_adc.Init.Channel = DMA_CHANNEL_0;
  hdma_adc.Init.Direction = DMA_PERIPH_TO_MEMORY;
  hdma_adc.Init.PeriphInc = DMA_PINC_DISABLE;
  hdma_adc.Init.MemInc = DMA_MINC_ENABLE;
  hdma_adc.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
  hdma_adc.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
  hdma_adc.Init.Mode = DMA_NORMAL;
  hdma_adc.Init.Priority = DMA_PRIORITY_HIGH;
  hdma_adc.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
  if ( HAL_DMA_Init( &hdma_adc ) != HAL_OK )
  {
    _Error_Handler( __FILE__, __LINE__ );
  }
	__HAL_LINKDMA( &hadc, DMA_Handle, hdma_adc );
	
	
	for( uint8_t i = 0; i < SHOCK_BUF_SECTIONS; ++i )
	{
		shock_buf_ptr[ i ] = shock_buf[ i ];
	}
	
	
	/*
	 * 40MHz -- APB1
	 * 80MHz -- TIM2
	 */
	__HAL_RCC_TIM2_CLK_ENABLE();
	htim.Instance = TIM2;
	htim.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	htim.Init.CounterMode= TIM_COUNTERMODE_UP;
	htim.Init.Period = 999;
	htim.Init.Prescaler = 80 - 1;
	if ( HAL_TIM_Base_Init( &htim ) != HAL_OK )
  {
    _Error_Handler( __FILE__, __LINE__ );
  }
	
	HAL_TIM_Base_Stop( &htim );
	
	
	TIM_MasterConfigTypeDef tim_trigger;
	tim_trigger.MasterOutputTrigger = TIM_TRGO_UPDATE;
	tim_trigger.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
	HAL_TIMEx_MasterConfigSynchronization( &htim, &tim_trigger );
	
	
	__HAL_DMA_CLEAR_FLAG( &hdma_adc, DMA_FLAG_HTIF0_4 );
	__HAL_DMA_CLEAR_FLAG( &hdma_adc, DMA_FLAG_TCIF0_4 );
	HAL_NVIC_SetPriority( DMA2_Stream0_IRQn, 0xA, 0x0 );
	HAL_NVIC_EnableIRQ( DMA2_Stream0_IRQn );
	
	
	if ( HAL_ADC_Start_DMA( &hadc, ( uint32_t* )shock_buf_ptr[ SHOCK_BUF_SECTIONS - 1 ], SHOCK_BUF_SIZE ) != HAL_OK )
  {
    _Error_Handler( __FILE__, __LINE__ );
  }
	
	//HAL_TIM_Base_Start( &htim );
	
	return;
}


void DMA2_Stream0_IRQHandler( void )
{
	if( __HAL_DMA_GET_FLAG( &hdma_adc, DMA_FLAG_HTIF0_4 ) != RESET )
	{
		__HAL_DMA_CLEAR_FLAG( &hdma_adc, DMA_FLAG_HTIF0_4 );
	}
	if( __HAL_DMA_GET_FLAG( &hdma_adc, DMA_FLAG_TCIF0_4 ) != RESET )
	{
		__HAL_DMA_CLEAR_FLAG( &hdma_adc, DMA_FLAG_TCIF0_4 );
		
		if ( HAL_ADC_Stop_DMA( &hadc ) != HAL_OK )
		{
			_Error_Handler( __FILE__, __LINE__ );
		}
		
		uint16_t* tmp = shock_buf_ptr[ 0 ];
		for( uint8_t i = 0; i < SHOCK_BUF_SECTIONS - 1; ++i )
					shock_buf_ptr[ i ] = shock_buf_ptr[ i + 1 ];
		shock_buf_ptr[ SHOCK_BUF_SECTIONS - 1 ] = tmp;
		
		if ( HAL_ADC_Start_DMA( &hadc, ( uint32_t* )shock_buf_ptr[ SHOCK_BUF_SECTIONS - 1 ], SHOCK_BUF_SIZE ) != HAL_OK )
		{
			_Error_Handler( __FILE__, __LINE__ );
		}
	}
	
	return;
}


uint16_t get_shock_data( void )
{
	return *( shock_buf_ptr[ SHOCK_BUF_SECTIONS - 1 ] + SHOCK_BUF_SIZE - __HAL_DMA_GET_COUNTER( &hdma_adc ) );
}


uint16_t** get_shock_buf( void )
{
	return shock_buf_ptr;
}


void flush_shock_buf( void )
{
	if ( HAL_ADC_Stop_DMA( &hadc ) != HAL_OK )
  {
    _Error_Handler( __FILE__, __LINE__ );
  }
	
	for( uint8_t i = 0; i < SHOCK_BUF_SECTIONS; ++i )
	{
		memset( shock_buf[ i ], 0, SHOCK_BUF_SIZE * 2 );
		shock_buf_ptr[ i ] = shock_buf[ i ];
	}
	
	__HAL_TIM_SET_COUNTER( &htim, 900 );
	
	if ( HAL_ADC_Start_DMA( &hadc, ( uint32_t* )shock_buf_ptr[ SHOCK_BUF_SECTIONS - 1 ], SHOCK_BUF_SIZE ) != HAL_OK )
  {
    _Error_Handler( __FILE__, __LINE__ );
  }
	
	return;
}
