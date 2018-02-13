#include "stm32f4xx_hal.h"
#include "mems/mems.h"
#include "mems/MPU9250_register_map.h"
#include <stdlib.h>
#include <string.h>


#define CS_ON()		HAL_GPIO_WritePin( GPIOB, GPIO_PIN_12, GPIO_PIN_RESET )
#define CS_OFF()	HAL_GPIO_WritePin( GPIOB, GPIO_PIN_12, GPIO_PIN_SET )

#define MPU9520_REGISTER_WRITE	0x00
#define MPU9520_REGISTER_READ		0x80



static SPI_HandleTypeDef hspi;

static uint8_t transmit_buffer[ 2 ] = { 0 };

static void spi_init( void );
static void dma_init( void );

static void read_registers	( mpu9520_register start, uint8_t num_to_read, uint8_t* receive_buffer );
static void write_register	( mpu9520_register reg, uint8_t value );

static void who_am_i		( uint8_t* recv_buf );
static void read_gyro		( uint8_t* recv_buf );
static void read_acc		( uint8_t* recv_buf );



void mems_init( void )
{
	spi_init();
	dma_init();
}


static void spi_init( void )
{
	__HAL_RCC_GPIOB_CLK_ENABLE();
	GPIO_InitTypeDef gpio;
	gpio.Alternate = GPIO_AF5_SPI2;
	gpio.Mode = GPIO_MODE_AF_PP;
	gpio.Pin = /*GPIO_PIN_12 |*/ GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
	gpio.Pull = GPIO_NOPULL;
	gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	HAL_GPIO_Init( GPIOB, &gpio );
	
	gpio.Mode = GPIO_MODE_OUTPUT_PP;
	gpio.Pin = GPIO_PIN_12;
	HAL_GPIO_Init( GPIOB, &gpio );
	CS_OFF();
	
	//PA9 -> OUT frameSync
	//PA8 -> EXTI from MPU
	
	__HAL_RCC_SPI2_CLK_ENABLE();
	hspi.Instance = SPI2;
	hspi.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_64;
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
	HAL_SPI_Init( &hspi );
	
	
	write_register( MPU9520_PWR_MGMT_1, 0x80 );
	
	return;
}



static void dma_init( void )
{
	return;
}


static void write_register	( mpu9520_register reg, uint8_t value )
{
	transmit_buffer[ 0 ] = MPU9520_REGISTER_WRITE | reg;
	transmit_buffer[ 1 ] = value;
	
	CS_ON();
	HAL_SPI_Transmit( &hspi, transmit_buffer, 2, HAL_MAX_DELAY );
	CS_OFF();
}


static void read_registers( mpu9520_register start, uint8_t num_to_read, uint8_t* receive_buffer )
{
	transmit_buffer[ 0 ] = MPU9520_REGISTER_READ | start;
	transmit_buffer[ 1 ] = 0x00;
	
	
	for( uint8_t i = 0; i < num_to_read; ++i )
	{
		CS_ON();
		HAL_SPI_Transmit( &hspi, transmit_buffer, 1, HAL_MAX_DELAY );
		HAL_SPI_Receive( &hspi, receive_buffer + i, 1, HAL_MAX_DELAY );
		CS_OFF();
		transmit_buffer[ 0 ] += 1;
	}
	
	if( num_to_read%2 == 0 )
	{
		for( uint8_t i = 0, temp = 0; i < 6; i += 2 )
		{
			temp = receive_buffer[ i ];
			receive_buffer[ i ] = receive_buffer[ i + 1 ];
			receive_buffer[ i + 1 ] = temp;
		}
	}
}




static void who_am_i( uint8_t* recv_buf )
{
	read_registers( MPU9520_WHO_AM_I, 1, recv_buf );
	return;
}

static void read_gyro( uint8_t* recv_buf )
{
	read_registers( MPU9520_GYRO_XOUT_H, 6, recv_buf );
	return;
}


static void read_acc( uint8_t* recv_buf )
{
	read_registers( MPU9520_ACCEL_XOUT_H, 6, recv_buf );
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
	}
	return;
}




