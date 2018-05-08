#include "stm32f4xx_hal.h"
#include "mems/mems.h"
#include "mems/MPU9250_register_map.h"
#include "console/console.h"
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#define CS_ON()			HAL_GPIO_WritePin( GPIOB, GPIO_PIN_12, GPIO_PIN_RESET )
#define CS_OFF()		HAL_GPIO_WritePin( GPIOB, GPIO_PIN_12, GPIO_PIN_SET )
#define SYNC_ON()		HAL_GPIO_WritePin( GPIOB, GPIO_PIN_12, GPIO_PIN_SET )
#define SYNC_OFF()	HAL_GPIO_WritePin( GPIOA, GPIO_PIN_9, GPIO_PIN_RESET )

#define MPU9520_FIFO_SIZE				0x012C
#define MPU9520_FIFO_MAX_SIZE		0x0200
#define FIFO_BUF_SECTIONS				4

#define REGISTER_ACTION_WRITE		0X00
#define REGISTER_ACTION_READ		0X80

#define MPU9520_ACCESS_ATTEMPTS 10
#define MPU9520_ACCESS_ATTEMPTS_DELAY 10
#define MPU9520_WAI_VALUE 			0x71

extern void _Error_Handler	(char * file, int line);


QueueHandle_t vMemsWorkProcQueue = NULL;

static SPI_HandleTypeDef hspi;
static DMA_HandleTypeDef hdma_spi_rx;
static DMA_HandleTypeDef hdma_spi_tx;

//using 40MHz on APB1 bus
static const uint32_t spi_1Mhz_BaudratePrescaler = SPI_BAUDRATEPRESCALER_64;
static const uint32_t spi_20Mhz_BaudratePrescaler = SPI_BAUDRATEPRESCALER_2;

static uint8_t spi_transmit_receive_buf[ MPU9520_FIFO_SIZE + 1 ] = { 0 };
static uint16_t fifo_buf[ FIFO_BUF_SECTIONS ][ MPU9520_FIFO_SIZE / 2 ] = { 0 };
static uint16_t *fifo_buf_ptr[ FIFO_BUF_SECTIONS ] = { NULL };

typedef enum
{
	FUNCTION_NAME_MEMS_IS_ACCESSABLE,
	FUNCTION_NAME_GET_ACC_GYRO_DATA,
	FUNCTION_NAME_FLUSH_FIFO_BUF
} function_name;

typedef struct
{
	TaskHandle_t task;
	function_name func_num;
	void* recv_buf;
} callback_struct;

static inline bool check_access_to_mems( void );
static inline void reverse_bytes( uint16_t* start, uint16_t num );



static void read_register( mpu9520_register start, uint8_t* receive_buf, uint16_t num_bytes_to_read );
static void write_register( mpu9520_register reg, uint8_t bitmask );


void vMemsInitProc( void* vParams )
{
	//gpio setup
	//PB13, PB14, PB15 -> SPI
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();
	GPIO_InitTypeDef gpio;
	gpio.Alternate = GPIO_AF5_SPI2;
	gpio.Mode = GPIO_MODE_AF_PP;
	gpio.Pin = GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
	gpio.Pull = GPIO_NOPULL;
	gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	HAL_GPIO_Init( GPIOB, &gpio );
	
	//PB12 -> CS
	gpio.Mode = GPIO_MODE_OUTPUT_PP;
	gpio.Pin = GPIO_PIN_12;
	HAL_GPIO_Init( GPIOB, &gpio );
	CS_OFF();
	
	//PA8 -> EXTI from MPU
	gpio.Mode = GPIO_MODE_IT_RISING;
	gpio.Pin = GPIO_PIN_8;
	HAL_GPIO_Init( GPIOA, &gpio );
#warning "EXTI INTERRUPT DISABLED!!!"
	//HAL_NVIC_SetPriority( EXTI9_5_IRQn, 5, 15 );
	//HAL_NVIC_EnableIRQ( EXTI9_5_IRQn );
	
	//PA9 -> OUT frameSync
	gpio.Mode = GPIO_MODE_OUTPUT_PP;
	gpio.Pin = GPIO_PIN_9;
	HAL_GPIO_Init( GPIOA, &gpio );
	SYNC_OFF();
	
	
	//spi setup
	__HAL_RCC_SPI2_CLK_ENABLE();
	hspi.Instance = SPI2;
	hspi.Init.BaudRatePrescaler = spi_1Mhz_BaudratePrescaler;
	hspi.Init.CLKPhase = SPI_PHASE_2EDGE;
	hspi.Init.CLKPolarity = SPI_POLARITY_HIGH;
	hspi.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
	hspi.Init.CRCPolynomial = 7;
	hspi.Init.DataSize = SPI_DATASIZE_8BIT;
	hspi.Init.Direction = SPI_DIRECTION_2LINES;
	hspi.Init.FirstBit = SPI_FIRSTBIT_MSB;
	hspi.Init.Mode = SPI_MODE_MASTER;
	hspi.Init.NSS = SPI_NSS_SOFT;
	hspi.Init.TIMode = SPI_TIMODE_DISABLE;
	if ( HAL_SPI_Init( &hspi ) != HAL_OK )
  {
    _Error_Handler( __FILE__, __LINE__ );
  }
	
	
	//dma setup
	__HAL_RCC_DMA1_CLK_ENABLE();
	hdma_spi_rx.Instance = DMA1_Stream3;
  hdma_spi_rx.Init.Channel = DMA_CHANNEL_0;
  hdma_spi_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
  hdma_spi_rx.Init.PeriphInc = DMA_PINC_DISABLE;
  hdma_spi_rx.Init.MemInc = DMA_MINC_ENABLE;
  hdma_spi_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
  hdma_spi_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
  hdma_spi_rx.Init.Mode = DMA_NORMAL;
  hdma_spi_rx.Init.Priority = DMA_PRIORITY_HIGH;
  hdma_spi_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
  if ( HAL_DMA_Init( &hdma_spi_rx ) != HAL_OK )
  {
    _Error_Handler( __FILE__, __LINE__ );
  }
	__HAL_LINKDMA( &hspi, hdmarx, hdma_spi_rx );
	
	__HAL_RCC_DMA1_CLK_ENABLE();
	hdma_spi_tx.Instance = DMA1_Stream4;
  hdma_spi_tx.Init.Channel = DMA_CHANNEL_0;
  hdma_spi_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
  hdma_spi_tx.Init.PeriphInc = DMA_PINC_DISABLE;
  hdma_spi_tx.Init.MemInc = DMA_MINC_ENABLE;
  hdma_spi_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
  hdma_spi_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
  hdma_spi_tx.Init.Mode = DMA_NORMAL;
  hdma_spi_tx.Init.Priority = DMA_PRIORITY_HIGH;
  hdma_spi_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
  if ( HAL_DMA_Init( &hdma_spi_tx ) != HAL_OK )
  {
    _Error_Handler( __FILE__, __LINE__ );
  }
	__HAL_LINKDMA( &hspi, hdmatx, hdma_spi_tx );
	
	
	//buf ptr setup
	for( uint8_t i = 0; i < FIFO_BUF_SECTIONS; ++i )
		fifo_buf_ptr[ i ] = fifo_buf[ i ];
	
	
	//Queue initial
	vMemsWorkProcQueue = xQueueCreate( 1, sizeof( callback_struct ) );
	if( vMemsWorkProcQueue == NULL )
	{
		_Error_Handler( __FILE__, __LINE__ );
	}
	
	
	vTaskDelete( NULL );
	vTaskDelay( 0 );
	return;
}

void vMemsWorkProc( void* vParams )
{
	//wait for power on
	vTaskDelay( 100 );
	
	
	send( "try to setup mpu", strlen( "try to setup mpu" ), true );
	
	
	if( !check_access_to_mems() )
	{
		send( "mpu not accessable before reset", strlen( "mpu not accessable before reset" ), true );
		while( true );
	}
	
	
	//reset mpu
	write_register( MPU9520_PWR_MGMT_1, BIT7 );
	vTaskDelay( 100 );
	
	
	//auto select clock source
	write_register( MPU9520_PWR_MGMT_1, ( 0x01 ) );
	//turn on all sensors
	write_register( MPU9520_PWR_MGMT_2, BIT_NONE );
	//wait for stable
  vTaskDelay( 200 );  
	
	
	if( !check_access_to_mems() )
	{
		send( "mpu not accessable after reset", strlen( "mpu not accessable after reset" ), true );
		while( true );
	}
	
	
	//reset fifo, gyro and acc, disable i2c slave module, enable fifo from spi
	write_register( MPU9520_USER_CTRL, BIT4 | BIT2 | BIT0 );
	
	
	//set LPF on Gyro to 92Hz and FSYNC
	/*
	//HAL_GPIO_WritePin( GPIOA, GPIO_PIN_9, GPIO_PIN_SET );
	for( uint8_t i = 2; i <= 7; ++i )
	{
		write_register( MPU9520_CONFIG, BIT6 | ( i << 3 ) | ( 0x01 ) );
	}
	//HAL_GPIO_WritePin( GPIOA, GPIO_PIN_9, GPIO_PIN_RESET );
	*/
	
	
	//set srdiv
	//write_register( MPU9520_SMPLRT_DIV, 249 ); //+1
	
	
	//set dlpf = 2, fifo dont write add data if it full
	write_register( MPU9520_CONFIG, BIT6 | ( 0x02 ) );
	
	
	//enable LPF -- ORate 1kHz
	write_register( MPU9520_GYRO_CONFIG, BIT_NONE );
	
	
	//set and enable LPF on Acc to 99Hz -- ORate 1kHz
	write_register( MPU9520_ACCEL_CONFIG_2, 0x02 );
	
	
	//setup pin_int to data ready
	write_register( MPU9520_INT_ENABLE, BIT0 );
	
	
	//enable fifo from spi
	write_register( MPU9520_USER_CTRL, BIT6 );
	
	
	//add to fifo Gyro and Acc
	write_register( MPU9520_FIFO_EN, BIT3 | BIT4 | BIT5 | BIT6 );
	send( "mems settings setup end", strlen( "mems settings setup end" ), true );
	
	
	uint16_t fifo_count = 0x0000;
	uint16_t* tmp = fifo_buf_ptr[ 0 ];
	callback_struct received;
	uint8_t acc_gyro_data[ 14 ] = { 0 };
	while( true )
	{
		if( xQueueReceive( vMemsWorkProcQueue, ( void* )&received, 10 ) == pdTRUE )
		{
			switch( received.func_num )
			{
				case FUNCTION_NAME_MEMS_IS_ACCESSABLE:
					*( bool* )received.recv_buf= check_access_to_mems();
					break;
				case FUNCTION_NAME_GET_ACC_GYRO_DATA:
					if( !check_access_to_mems() ) break;
					read_register( MPU9520_ACCEL_XOUT_H, acc_gyro_data, 14 );
					reverse_bytes( ( uint16_t* )acc_gyro_data, 14 );
					memcpy( ( uint8_t* )received.recv_buf, acc_gyro_data, 2 * 3 );
					memcpy( ( uint8_t* )received.recv_buf + ( 2 * 3 ), acc_gyro_data + ( 2 * 4 ), 2 * 3 );
					break;
				case FUNCTION_NAME_FLUSH_FIFO_BUF:
					for( uint8_t i = 0; i < FIFO_BUF_SECTIONS; ++i )
					{
						fifo_buf_ptr[ i ] = fifo_buf[ i ];
						memset( fifo_buf[ i ], 0, MPU9520_FIFO_SIZE );
					}
					read_register( MPU9520_FIFO_R_W, NULL, MPU9520_FIFO_SIZE );
					break;
			}
			xTaskNotifyGive( received.task );
		}
		
		read_register( MPU9520_FIFO_COUNTH, ( uint8_t* )&fifo_count, 2 );
		reverse_bytes( &fifo_count, 2 );
		
		if( fifo_count >= MPU9520_FIFO_SIZE && fifo_count <= MPU9520_FIFO_MAX_SIZE )
		{
			read_register( MPU9520_FIFO_R_W, ( uint8_t* )fifo_buf_ptr[ FIFO_BUF_SECTIONS - 1 ], MPU9520_FIFO_SIZE );
			//reverse_bytes( fifo_buf_ptr[ FIFO_BUF_SECTIONS - 1 ], MPU9520_FIFO_SIZE );
			tmp = fifo_buf_ptr[ 0 ];
			for( uint8_t i = 0; i < FIFO_BUF_SECTIONS - 1; ++i )
				fifo_buf_ptr[ i ] = fifo_buf_ptr[ i + 1 ];
			fifo_buf_ptr[ FIFO_BUF_SECTIONS - 1 ] = tmp;
		}
	}
}


void vMemsSyncProc( void* vParams )
{
	while( true )
	{
		SYNC_ON();
		SYNC_OFF();
		vTaskDelay(1);
	}
}


static inline void reverse_bytes( uint16_t* start, uint16_t num )
{
	for( uint16_t i = 0; i < num / 2; ++i )
		*( start + i ) = ( *( start + i ) << 8 ) + ( *( start + i ) >> 8 );
	return;
}


static bool check_access_to_mems( void )
{
	uint8_t tmp = 0x00;
	for( uint16_t i = 0; i < MPU9520_ACCESS_ATTEMPTS; ++i )
	{
		read_register( MPU9520_WHO_AM_I, &tmp, 1 );
		if( tmp == MPU9520_WAI_VALUE ) return true;
		else vTaskDelay( MPU9520_ACCESS_ATTEMPTS_DELAY );
	}
	return false;
}





static void write_register( mpu9520_register reg, uint8_t bitmask )
{
	while( __HAL_SPI_GET_FLAG( &hspi, SPI_FLAG_BSY ) ) ;
	
	if( hspi.Init.BaudRatePrescaler != spi_1Mhz_BaudratePrescaler )
	{
		if ( HAL_SPI_DeInit( &hspi ) != HAL_OK )
		{
			_Error_Handler( __FILE__, __LINE__ );
		}
		hspi.Init.BaudRatePrescaler = spi_1Mhz_BaudratePrescaler;
		if ( HAL_SPI_Init( &hspi ) != HAL_OK )
		{
			_Error_Handler( __FILE__, __LINE__ );
		}
	}
	
	memset( spi_transmit_receive_buf, 0xFF, 2 );
	spi_transmit_receive_buf[ 0 ] = REGISTER_ACTION_WRITE | reg;
	spi_transmit_receive_buf[ 1 ] = bitmask;
	
	CS_ON();
	
	if ( HAL_SPI_Transmit( &hspi, spi_transmit_receive_buf, 2, 100 ) != HAL_OK )
	{
		_Error_Handler( __FILE__, __LINE__ );
	}
	
	CS_OFF();
	
	return;
}

static void read_register( mpu9520_register start, uint8_t* receive_buf, uint16_t num_bytes_to_read )
{
	while( __HAL_SPI_GET_FLAG( &hspi, SPI_FLAG_BSY ) ) ;
	
	if( hspi.Init.BaudRatePrescaler != spi_20Mhz_BaudratePrescaler )
	{
		if ( HAL_SPI_DeInit( &hspi ) != HAL_OK )
		{
			_Error_Handler( __FILE__, __LINE__ );
		}
		hspi.Init.BaudRatePrescaler = spi_20Mhz_BaudratePrescaler;
		if ( HAL_SPI_Init( &hspi ) != HAL_OK )
		{
			_Error_Handler( __FILE__, __LINE__ );
		}
	}
	
	memset( spi_transmit_receive_buf, 0x00, num_bytes_to_read + 1 );
	spi_transmit_receive_buf[ 0 ] = REGISTER_ACTION_READ | start;
	
	CS_ON();
	
	if ( HAL_SPI_TransmitReceive( &hspi, spi_transmit_receive_buf, spi_transmit_receive_buf, num_bytes_to_read + 1, 100 ) )
	{
		_Error_Handler( __FILE__, __LINE__ );
	}
	
	CS_OFF();
	
	if( receive_buf != NULL )
		memcpy( receive_buf, spi_transmit_receive_buf + 1, num_bytes_to_read );
	
	return;
}





bool mems_is_accessable( void )
{
	bool is_accessable = false;
	callback_struct toDo;
	toDo.func_num = FUNCTION_NAME_MEMS_IS_ACCESSABLE;
	toDo.recv_buf = ( void* )&is_accessable;
	toDo.task = xTaskGetCurrentTaskHandle();
	xQueueSend( vMemsWorkProcQueue, ( void* )&toDo, portMAX_DELAY );
	
	ulTaskNotifyTake( pdTRUE, portMAX_DELAY );
	if( is_accessable ) return true;
	else return false;
}

void get_acc_gyro_data( uint8_t* recv_buf )
{
	callback_struct toDo;
	toDo.func_num = FUNCTION_NAME_GET_ACC_GYRO_DATA;
	toDo.recv_buf = ( void* )recv_buf;
	toDo.task = xTaskGetCurrentTaskHandle();
	xQueueSend( vMemsWorkProcQueue, ( void* )&toDo, portMAX_DELAY );
	
	ulTaskNotifyTake( pdTRUE, portMAX_DELAY );
	return;
}

void flush_fifo_buf( void )
{
	callback_struct toDo;
	toDo.func_num = FUNCTION_NAME_FLUSH_FIFO_BUF;
	toDo.recv_buf = NULL;
	toDo.task = xTaskGetCurrentTaskHandle();
	xQueueSend( vMemsWorkProcQueue, ( void* )&toDo, portMAX_DELAY );
	
	ulTaskNotifyTake( pdTRUE, portMAX_DELAY );
	return;
}

uint16_t** get_acc_gyro_buf( void )
{
	return fifo_buf_ptr;
}
