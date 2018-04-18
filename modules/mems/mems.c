#include "stm32f4xx_hal.h"
#include "mems/mems.h"
#include "mems/MPU9250_register_map.h"
#include "timers/timers.h"
#include "console/console.h"
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#define CS_ON()		HAL_GPIO_WritePin( GPIOB, GPIO_PIN_12, GPIO_PIN_RESET )
#define CS_OFF()	HAL_GPIO_WritePin( GPIOB, GPIO_PIN_12, GPIO_PIN_SET )

#define MPU9520_REGISTER_WRITE	0x00
#define MPU9520_REGISTER_READ		0x80

#define MPU9520_FIFO_SIZE				0x012C
#define MPU9520_FIFO_MAX_SIZE		0x0200
#define FIFO_BANK_SECTIONS			4

#define MPU9520_ACCESS_ATTEMPTS 10
#define MPU9520_ACCESS_ATTEMPTS_DELAY 10
#define MPU9520_WAI_VALUE 			0x71


extern void _Error_Handler	(char * file, int line);


static SPI_HandleTypeDef hspi;
static DMA_HandleTypeDef hdma_spi_rx;
static DMA_HandleTypeDef hdma_spi_tx;

static uint8_t spi_transmit_receive_buffer[ MPU9520_FIFO_SIZE + 1 ] = { 0 };
static uint16_t fifo_bank[ FIFO_BANK_SECTIONS ][ MPU9520_FIFO_SIZE / 2 ] = { 0 };
static uint16_t *fifo_bank_ptr[ FIFO_BANK_SECTIONS ] = { NULL };
static uint8_t new_data = 0x00;


static void spi_init( uint32_t spiMaxSpeedInKHz );
static void dma_init( void );
static void periph_init( void );

void mems_sync( void );
void print_fifo_proc( void );
void mems_read_fifo_proc( void );

static void read_register( mpu9520_register start, uint8_t* receive_buffer, uint16_t num_bytes_to_read );
static void write_register( mpu9520_register reg, uint8_t bitmask );

static void who_am_i		( uint8_t* recv_buf );
static void read_gyro		( uint8_t* recv_buf );
static void read_acc		( uint8_t* recv_buf );
static void read_reg		( uint8_t* recv_buf );




void mems_init( void )
{
	for( uint8_t i = 0; i < FIFO_BANK_SECTIONS; ++i )
	{
		fifo_bank_ptr[ i ] = fifo_bank[ i ];
	}
	
	spi_init( 1000 );
	dma_init();
	periph_init();
	spi_init( 20000 );
	
	add_task( mems_sync, 1 );
	//add_task( print_fifo_proc, 10 );
	//add_task( mems_read_fifo_proc, 5 );
}


static void spi_init( uint32_t spiMaxSpeedInKHz )
{
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
	
#warning "EXTI9_5_IRQn"
	//PA8 -> EXTI from MPU
	
	gpio.Mode = GPIO_MODE_IT_RISING;
	gpio.Pin = GPIO_PIN_8;
	HAL_GPIO_Init( GPIOA, &gpio );
	HAL_NVIC_SetPriority( EXTI9_5_IRQn, 10, 15 );
	HAL_NVIC_EnableIRQ( EXTI9_5_IRQn );
	
	
	//PA9 -> OUT frameSync
	gpio.Mode = GPIO_MODE_OUTPUT_PP;
	gpio.Pin = GPIO_PIN_9;
	HAL_GPIO_Init( GPIOA, &gpio );
	HAL_GPIO_WritePin( GPIOA, GPIO_PIN_9, GPIO_PIN_RESET );
	
	__HAL_RCC_SPI2_CLK_ENABLE();
	hspi.Instance = SPI2;
	
	//using 40MHz on APB1 bus
	if( spiMaxSpeedInKHz >= 20000 )
		hspi.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
	else if( spiMaxSpeedInKHz >= 10000 )
		hspi.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
	else if( spiMaxSpeedInKHz >= 5000 )
		hspi.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
	else if( spiMaxSpeedInKHz >= 2500 )
		hspi.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
	else if( spiMaxSpeedInKHz >= 1250 )
		hspi.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;
	else if( spiMaxSpeedInKHz >= 625 )
		hspi.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_64;
	else if( spiMaxSpeedInKHz >= 312 )
		hspi.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_128;
	else if( spiMaxSpeedInKHz >= 156 )
		hspi.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;
	else
		_Error_Handler( __FILE__, __LINE__ );
	
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
	if ( HAL_SPI_DeInit( &hspi ) != HAL_OK )
  {
    _Error_Handler( __FILE__, __LINE__ );
  }
	if ( HAL_SPI_Init( &hspi ) != HAL_OK )
  {
    _Error_Handler( __FILE__, __LINE__ );
  }
	
	return;
}



static void dma_init( void )
{
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
	
	HAL_SPI_DMAStop( &hspi );
	
	return;
}


void mems_sync( void )
{
	//HAL_GPIO_TogglePin( GPIOA, GPIO_PIN_9 );
	HAL_GPIO_WritePin( GPIOA, GPIO_PIN_9, GPIO_PIN_SET );
	HAL_GPIO_WritePin( GPIOA, GPIO_PIN_9, GPIO_PIN_RESET );
	return;
}


void mems_read_fifo_proc( void )
{
	uint16_t fifo_count = 0x0000;
	read_register( MPU9520_FIFO_COUNTH, ( uint8_t* )&fifo_count, 2 );
	fifo_count = ( fifo_count >> 8 ) + ( fifo_count << 8 );
	
	/*
	char tmp_time[ 20 ] = { 0 };
	sprintf( tmp_time, "%3d at %d", fifo_count, get_global_time() );
	send( tmp_time, strlen( tmp_time ), true );
	*/
	
	if( fifo_count >= MPU9520_FIFO_SIZE && fifo_count <= MPU9520_FIFO_MAX_SIZE )
	{
		uint16_t *tmp = fifo_bank_ptr[ 0 ];
		for( uint8_t i = 0; i < FIFO_BANK_SECTIONS - 1; ++i )
		{
			fifo_bank_ptr[ i ] = fifo_bank_ptr[ i + 1 ];
		}
		fifo_bank_ptr[ FIFO_BANK_SECTIONS - 1 ] = tmp;
		read_register( MPU9520_FIFO_R_W, ( uint8_t* )fifo_bank_ptr[ FIFO_BANK_SECTIONS - 1 ], MPU9520_FIFO_SIZE );
		if( new_data < FIFO_BANK_SECTIONS - 1 ) ++new_data;
	}
	
	return;
}



void EXTI9_5_IRQHandler( void )
{
	__HAL_GPIO_EXTI_CLEAR_IT( GPIO_PIN_8 );
	
	static uint32_t timestamp = 0x00000000;
	char tmp[ 20 ] = { 0 };
	
	sprintf( tmp, "data ready at %dms", get_global_time() - timestamp );
	timestamp = get_global_time();
	
	send( tmp, strlen( tmp ), true );
	
	return;
}


static void periph_init( void )
{
	//wait for power on
	delay_ms( 200 );
	
	send( "try to setup mpu", strlen( "try to setup mpu" ), true );
	if( !mems_is_accessable() )
	{
		send( "mpu not accessable before reset", strlen( "mpu not accessable before reset" ), true );
		return;
	}
	send( "mpu accessable before reset", strlen( "mpu accessable before reset" ), true );
	
	//reset mpu and set osc clock source
	write_register( MPU9520_PWR_MGMT_1, BIT7 );
	//set osc to pll
	write_register( MPU9520_PWR_MGMT_1, ( 0x02 ) );
	
	send( "reset mpu", strlen( "reset mpu" ), true );
	
	if( !mems_is_accessable() )
	{
		send( "mpu not accessable after reset", strlen( "mpu not accessable after reset" ), true );
		return;
	}
	send( "mpu accessable after reset", strlen( "mpu accessable after reset" ), true );
	
	//reset fifo, gyro and acc, disable i2c slave module, enable fifo from spi
	write_register( MPU9520_USER_CTRL, BIT6 | BIT4 | BIT2 | BIT0 );
	send( "reset periph", strlen( "reset periph" ), true );
	
	
	//set LPF on Gyro to 92Hz and FSYNC
	/*
	HAL_GPIO_WritePin( GPIOA, GPIO_PIN_9, GPIO_PIN_SET );
	for( uint8_t i = 2; i <= 7; ++i )
	{
		write_register( MPU9520_CONFIG, BIT6 | ( i << 3 ) | ( 0x01 ) );
		send( "set config", strlen( "set config" ), true );
	}
	HAL_GPIO_WritePin( GPIOA, GPIO_PIN_9, GPIO_PIN_RESET );
	*/
	
	// set srdiv
	write_register( MPU9520_SMPLRT_DIV, 249 );
	send( "set srdiv", strlen( "set srdiv" ), true );
	
	write_register( MPU9520_CONFIG, BIT6 | ( 0x02 ) );
	send( "set config", strlen( "set config" ), true );
	
	//enable LPF -- ORate 1kHz
	//write_register( MPU9520_GYRO_CONFIG, BIT_NONE );
	//send( "set gyro config", strlen( "set gyro config" ), true );
	
	//set and enable LPF on Acc to 99Hz -- ORate 1kHz
	write_register( MPU9520_ACCEL_CONFIG_2, 0x02 );
	send( "set acc config", strlen( "set acc config" ), true );
	
	//setup pin_int to data ready
	write_register( MPU9520_INT_ENABLE, BIT0 );
	send( "set intpin to data ready", strlen( "set intpin to data ready" ), true );
	
	//add to fifo Gyro and Acc
	//write_register( MPU9520_FIFO_EN, /*BIT3 | BIT4 | BIT5 | BIT6*/ BIT7 );
	//send( "fifo enable", strlen( "fifo enable" ), true );
	send( "setup end", strlen( "setup end" ), true );
	
	return;
}


bool mems_is_accessable( void )
{
	uint8_t tmp = 0x00;
	for( uint16_t i = 0; i < MPU9520_ACCESS_ATTEMPTS; ++i )
	{
		who_am_i( &tmp );
		if( tmp == MPU9520_WAI_VALUE ) return true;
		else delay_ms( MPU9520_ACCESS_ATTEMPTS_DELAY );
	}
	return false;
}



static void write_register( mpu9520_register reg, uint8_t bitmask )
{
	while( __HAL_SPI_GET_FLAG( &hspi, SPI_FLAG_BSY ) ) ;
	
	memset( spi_transmit_receive_buffer, 0xFF, 2 );
	spi_transmit_receive_buffer[ 0 ] = MPU9520_REGISTER_WRITE | reg;
	spi_transmit_receive_buffer[ 1 ] = bitmask;
	
	CS_ON();
	if ( HAL_SPI_Transmit( &hspi, spi_transmit_receive_buffer, 2, 100 ) != HAL_OK )
	//if ( HAL_SPI_Transmit_DMA( &hspi, spi_transmit_receive_buffer, 2 ) != HAL_OK )
	{
		_Error_Handler( __FILE__, __LINE__ );
	}
	while( __HAL_SPI_GET_FLAG( &hspi, SPI_FLAG_BSY ) ) ;
	//HAL_SPI_DMAStop( &hspi );
	CS_OFF();
	
	return;
}


static void read_register( mpu9520_register start, uint8_t* receive_buffer, uint16_t num_bytes_to_read )
{
	while( __HAL_SPI_GET_FLAG( &hspi, SPI_FLAG_BSY ) ) ;
	
	memset( spi_transmit_receive_buffer, 0xFF, num_bytes_to_read + 1 );
	spi_transmit_receive_buffer[ 0 ] = MPU9520_REGISTER_READ | start;
	
	CS_ON();
	if ( HAL_SPI_TransmitReceive_DMA( &hspi, spi_transmit_receive_buffer, spi_transmit_receive_buffer, num_bytes_to_read + 1 ) != HAL_OK )
	//if ( HAL_SPI_TransmitReceive( &hspi, spi_transmit_receive_buffer, spi_transmit_receive_buffer, num_bytes_to_read + 1, 100 ) != HAL_OK )
	{
		_Error_Handler( __FILE__, __LINE__ );
	}
	while( __HAL_SPI_GET_FLAG( &hspi, SPI_FLAG_BSY ) ) ;
	HAL_SPI_DMAStop( &hspi );
	CS_OFF();
	
	memcpy( receive_buffer, spi_transmit_receive_buffer + 1, num_bytes_to_read );
	return;
}




static void who_am_i( uint8_t* recv_buf )
{
	read_register( MPU9520_WHO_AM_I, recv_buf, 1 );
	return;
}

static void read_gyro( uint8_t* recv_buf )
{
	if( !mems_is_accessable() ) return;
	
	read_register( MPU9520_GYRO_XOUT_L, recv_buf, 1 );
	read_register( MPU9520_GYRO_XOUT_H, recv_buf + 1, 1 );
	read_register( MPU9520_GYRO_YOUT_L, recv_buf + 2, 1 );
	read_register( MPU9520_GYRO_YOUT_H, recv_buf + 3, 1 );
	read_register( MPU9520_GYRO_ZOUT_L, recv_buf + 4, 1 );
	read_register( MPU9520_GYRO_ZOUT_H, recv_buf + 5, 1 );
	return;
}

static void read_acc( uint8_t* recv_buf )
{
	if( !mems_is_accessable() ) return;
	
	read_register( MPU9520_ACCEL_XOUT_L, recv_buf, 1 );
	read_register( MPU9520_ACCEL_XOUT_H, recv_buf + 1, 1 );
	read_register( MPU9520_ACCEL_YOUT_L, recv_buf + 2, 1 );
	read_register( MPU9520_ACCEL_YOUT_H, recv_buf + 3, 1 );
	read_register( MPU9520_ACCEL_ZOUT_L, recv_buf + 4, 1 );
	read_register( MPU9520_ACCEL_ZOUT_H, recv_buf + 5, 1 );
	return;
}

static void read_reg( uint8_t* recv_buf )
{
	if( !mems_is_accessable() ) return;
	read_register( atoi( ( char* )recv_buf ), recv_buf, 1 );
	return;
}

void get_mems_data( data_name name, uint8_t* recv_buf )
{
	switch( name )
	{
		case WHOAMI:
			who_am_i( recv_buf );
			break;
		case GYROSCOPE:
			read_gyro( recv_buf );
			break;
		case ACCELEROMETER:
			read_acc( recv_buf );
			break;
		case READREG:
			read_reg( recv_buf );
			break;
	}
	return;
}





static bool print = false;
static char format = 'h';
static bool full = false;
static bool endless = false;

void print_fifo( bool new_print, char *new_format, bool new_full, bool new_endless )
{
	print = new_print;
	
	if( new_format != NULL )
	{
		if( strcmp( new_format, "HEX" ) == 0 ) format = 'h';
		else if( strcmp( new_format, "DEC" ) == 0 ) format = 'd';
	}
	
	full = new_full;
	
	endless = new_endless;
	
	return;
}

void print_fifo_proc( void )
{
	if( is_send() ) return;
	if( !print ) return;
	print = false;
	
	
	if( new_data )
	{
		char str[ 80 ] = { 0 };
		uint16_t *start = 0;
		
		if( full )
		{
			start = fifo_bank_ptr[ FIFO_BANK_SECTIONS - new_data ];
			if( new_data ) --new_data;
		}
		else
		{
			start = fifo_bank_ptr[ FIFO_BANK_SECTIONS - 1 ];
			new_data = 0;
		}
		
		while( __HAL_SPI_GET_FLAG( &hspi, SPI_FLAG_BSY ) ) ;
		
		
		
		for( uint16_t i = 0; i < MPU9520_FIFO_SIZE / 2; i += 6 )
		{
			if( format == 'h' )
				sprintf( str, "GX:%4hX,GY:%4hX,GZ:%4hX,AX:%4hX,AY:%4hX,AZ:%4hX\n",
								*( start + i + 0 ),
								*( start + i + 1 ),
								*( start + i + 2 ),
								*( start + i + 3 ),
								*( start + i + 4 ),
								*( start + i + 5 ) );
			else if( format == 'd' )
				sprintf( str, "GX:%6hd,GY:%6hd,GZ:%6hd,AX:%6hd,AY:%6hd,AZ:%6hd",
								*( start + i + 0 ),
								*( start + i + 1 ),
								*( start + i + 2 ),
								*( start + i + 3 ),
								*( start + i + 4 ),
								*( start + i + 5 ) );
			send( str, strlen( str ), true );
		}
	}
	
	if( endless ) print = true;
	
	return;
}



