#include "console/console.h"
#include "stm32f4xx_hal.h"
#include <stdlib.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#define RX_DMA_BUFFER_SIZE 64
#define MAX_COMMAND_PARAMETERS 16

extern void _Error_Handler( char * file, int line );


static char* RX_DMA_BUFFER = NULL;
static char* TX_DMA_BUFFER = NULL;


static CRC_HandleTypeDef hcrc;
static UART_HandleTypeDef huart;
static DMA_HandleTypeDef hdma_uart_rx;
static DMA_HandleTypeDef hdma_uart_tx;

TaskHandle_t vConsoleWorkProcHandle = NULL;
SemaphoreHandle_t vConsoleSendSemaphore = NULL;

static inline void clear_rx_dma_buffer( void );


void vConsoleInitProc( void* vParams )
{
	//configure gpio
	__HAL_RCC_GPIOA_CLK_ENABLE();
	GPIO_InitTypeDef gpio;
	gpio.Alternate = GPIO_AF7_USART2;
	gpio.Mode = GPIO_MODE_AF_PP;
	gpio.Pin = GPIO_PIN_2 | GPIO_PIN_3;
	gpio.Pull = GPIO_PULLUP;
	gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	HAL_GPIO_Init( GPIOA, &gpio );
	
	//configure usart
	__HAL_RCC_USART2_CLK_ENABLE();
	huart.Instance = USART2;
  huart.Init.BaudRate = 256000;
  huart.Init.WordLength = UART_WORDLENGTH_8B;
  huart.Init.StopBits = UART_STOPBITS_1;
  huart.Init.Parity = UART_PARITY_NONE;
  huart.Init.Mode = UART_MODE_TX_RX;
  huart.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart.Init.OverSampling = UART_OVERSAMPLING_16;
  if ( HAL_UART_Init( &huart ) != HAL_OK )
  {
    _Error_Handler( __FILE__, __LINE__ );
  }
	
	//configure dma rx
	__HAL_RCC_DMA1_CLK_ENABLE();
	hdma_uart_rx.Instance = DMA1_Stream5;
  hdma_uart_rx.Init.Channel = DMA_CHANNEL_4;
  hdma_uart_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
  hdma_uart_rx.Init.PeriphInc = DMA_PINC_DISABLE;
  hdma_uart_rx.Init.MemInc = DMA_MINC_ENABLE;
  hdma_uart_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
  hdma_uart_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
  hdma_uart_rx.Init.Mode = DMA_NORMAL;
  hdma_uart_rx.Init.Priority = DMA_PRIORITY_LOW;
  hdma_uart_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
  if ( HAL_DMA_Init( &hdma_uart_rx ) != HAL_OK )
  {
    _Error_Handler( __FILE__, __LINE__ );
  }
	__HAL_LINKDMA( &huart, hdmarx, hdma_uart_rx );
	
	//configure dma tx
	hdma_uart_tx.Instance = DMA1_Stream6;
  hdma_uart_tx.Init.Channel = DMA_CHANNEL_4;
  hdma_uart_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
  hdma_uart_tx.Init.PeriphInc = DMA_PINC_DISABLE;
  hdma_uart_tx.Init.MemInc = DMA_MINC_ENABLE;
  hdma_uart_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
  hdma_uart_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
  hdma_uart_tx.Init.Mode = DMA_NORMAL;
  hdma_uart_tx.Init.Priority = DMA_PRIORITY_LOW;
  hdma_uart_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
  if ( HAL_DMA_Init( &hdma_uart_tx ) != HAL_OK )
  {
    _Error_Handler( __FILE__, __LINE__ );
  }
	__HAL_LINKDMA( &huart, hdmatx, hdma_uart_tx );
	
	RX_DMA_BUFFER = ( char* )pvPortMalloc( RX_DMA_BUFFER_SIZE );
	memset( RX_DMA_BUFFER, 0, RX_DMA_BUFFER_SIZE );
	
	
	__HAL_UART_CLEAR_FLAG( &huart, UART_FLAG_TC );
	__HAL_UART_CLEAR_IDLEFLAG( &huart );
	__HAL_UART_ENABLE_IT( &huart, USART_IT_TC | UART_IT_IDLE );
	HAL_NVIC_SetPriority( USART2_IRQn, 0xE, 0x0 );
  HAL_NVIC_EnableIRQ( USART2_IRQn );
	
	
	if ( HAL_UART_Receive_DMA( &huart, ( uint8_t* )RX_DMA_BUFFER, RX_DMA_BUFFER_SIZE ) != HAL_OK )
  {
    _Error_Handler( __FILE__, __LINE__ );
  }
	
	 if( ( vConsoleSendSemaphore = xSemaphoreCreateBinary() ) == NULL )
		 _Error_Handler( __FILE__, __LINE__ );
	 xSemaphoreGive( vConsoleSendSemaphore );
	
	//initial message
	send( "THIS IS ALPHA VERSION SABRE PROJECT",
				strlen("THIS IS ALPHA VERSION SABRE PROJECT"), true );
	
	
	vTaskDelete( NULL );
	vTaskDelay( 0 );
	return;
}


void vConsoleWorkProc( void* vParams )
{
	extern const char *FUNCTIONS_NAMES[];
	extern void ( *FUNCTIONS_LIST[] )( int, char** );
	
	char* temp = NULL;
	char* message_ptr = RX_DMA_BUFFER + 1;
	int argc = 0;
	char *argv[ 10 ] = { NULL };
	
	while( true )
	{
		ulTaskNotifyTake( pdTRUE, portMAX_DELAY );
		
		if( *RX_DMA_BUFFER != 'M' || strchr( RX_DMA_BUFFER, '\n' ) == NULL )
		{
			clear_rx_dma_buffer();
			continue;
		}
		
		message_ptr = RX_DMA_BUFFER + 1;
		argc = 0;
		
		while( true )
		{
			if( ( temp = strchr( message_ptr, ',' ) ) == NULL )
			{
				temp = strchr( message_ptr, '\n' );
				++argc;
				if( argc == MAX_COMMAND_PARAMETERS ) _Error_Handler( __FILE__, __LINE__ ); 
				argv[ argc - 1 ] = ( char* )pvPortMalloc( temp - message_ptr + 1 );
				memset( argv[ argc - 1 ], 0, temp - message_ptr + 1 );
				memcpy( argv[ argc - 1 ], message_ptr, temp - message_ptr );
				
				//call function!!!
				for( unsigned int i = 0; FUNCTIONS_NAMES[ i ] != NULL; ++i )
				{
					clear_rx_dma_buffer();
					if( strcmp( argv[ 0 ], FUNCTIONS_NAMES[ i ] ) == 0 )
					{
						( *FUNCTIONS_LIST[ i ] )( argc, argv );
						break;
					}
					if( FUNCTIONS_NAMES[ i + 1 ] == NULL )
						send( "404", strlen( "404" ), true );
				}
				break;
			}
			++argc;
			argv[ argc - 1 ] = ( char* )pvPortMalloc( temp - message_ptr + 1 );
			memset( argv[ argc - 1 ], 0, temp - message_ptr + 1 );
			memcpy( argv[ argc - 1 ], message_ptr, temp - message_ptr );
			message_ptr = temp + 1;
		}
		
		for( int i = 0; i < argc; ++i )
		{
			vPortFree( argv[ i ] );
			argv[ i ] = NULL;
		}
	}
}


void send( char* data, unsigned short len, bool raw )
{
	xSemaphoreTake( vConsoleSendSemaphore, portMAX_DELAY );
	
	if( !raw ) len = len + 2;
	else len = len + 1;
	
	if( TX_DMA_BUFFER != NULL )
	{
		vPortFree( TX_DMA_BUFFER );
		TX_DMA_BUFFER = NULL;
	}
	TX_DMA_BUFFER = ( char* )pvPortMalloc( len );
	
	if( TX_DMA_BUFFER == NULL )
	{
		send( "MEMFAULT", strlen( "MEMFAULT" ), false );
		return;
	}
	else
	{
		if( !raw )
		{
			memset( TX_DMA_BUFFER, 0, len );
			*TX_DMA_BUFFER = 'S';
			strncat( TX_DMA_BUFFER, data, len - 2 );
		}
		else
		{
			memset( TX_DMA_BUFFER, 0, len );
			strncat( TX_DMA_BUFFER, data, len - 1 );
		}
	}
	
	strncat( TX_DMA_BUFFER, "\n", 1 );
	
	if ( HAL_UART_Transmit_DMA( &huart, ( uint8_t* )TX_DMA_BUFFER, len ) != HAL_OK )
  {
    _Error_Handler( __FILE__, __LINE__ );
  }
	
	return;
}


static inline void clear_rx_dma_buffer( void )
{
	if ( HAL_UART_AbortReceive( &huart ) != HAL_OK )
  {
    _Error_Handler( __FILE__, __LINE__ );
  }
	memset( RX_DMA_BUFFER, 0, RX_DMA_BUFFER_SIZE );
	if ( HAL_UART_Receive_DMA( &huart, ( uint8_t* )RX_DMA_BUFFER, RX_DMA_BUFFER_SIZE ) != HAL_OK )
  {
    _Error_Handler( __FILE__, __LINE__ );
  }
	return;
}


void USART2_IRQHandler( void )
{
	if( __HAL_UART_GET_FLAG( &huart, UART_FLAG_TC ) )
	{
		__HAL_UART_CLEAR_FLAG( &huart, UART_FLAG_TC );
		if ( HAL_UART_AbortTransmit( &huart ) != HAL_OK )
		{
			_Error_Handler( __FILE__, __LINE__ );
		}
		__HAL_UART_ENABLE_IT( &huart, USART_IT_TC );
		xSemaphoreGiveFromISR( vConsoleSendSemaphore, NULL );
	}
	if( __HAL_UART_GET_FLAG( &huart, UART_FLAG_IDLE ) )
	{
		__HAL_UART_CLEAR_IDLEFLAG( &huart );
		vTaskNotifyGiveFromISR( vConsoleWorkProcHandle, NULL );
	}
	
	return;
}






