#include "stm32f4xx_hal.h"
#include "mems/mems.h"
#include "mems/MPU9250_register_map.h"
#include <stdlib.h>
#include <string.h>


#define CS_ON() HAL_GPIO_WritePin( GPIOB, GPIO_PIN_12, GPIO_PIN_RESET );
#define CS_OFF() HAL_GPIO_WritePin( GPIOB, GPIO_PIN_12, GPIO_PIN_SET );


static uint8_t FUNCTION_ID_current = 0;
#define FUNCTION_ID_who_am_i							1
#define FUNCTION_ID_read_gyro							6
#define FUNCTION_ID_read_acc							6


typedef enum
{
	MPU9520_REGISTER_READ,
	MPU9520_REGISTER_WRITE
} mpu9520_register_mode;



static SPI_HandleTypeDef hspi;

static uint8_t transmit_buffer[ 2 ] = { 0 };
static uint8_t* receive_buffer = NULL;

static void create_message( mpu9520_register_mode mode, mpu9520_register adress, uint8_t data );


void mems_init( void )
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
	HAL_GPIO_WritePin( GPIOB, GPIO_PIN_12, GPIO_PIN_SET );
	
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
	
	HAL_SPI_Transmit( &hspi, transmit_buffer, 2, HAL_MAX_DELAY );
	
	return;
}

static void create_message( mpu9520_register_mode mode, mpu9520_register adress, uint8_t data )
{
	switch( mode )
	{
		case MPU9520_REGISTER_READ:
			transmit_buffer[ 0 ] = 0x80 + adress;
			break;
		case MPU9520_REGISTER_WRITE:
			transmit_buffer[ 0 ] = adress;
			break;
	}
	transmit_buffer[ 1 ] = data;
}

static void memory_allocation_receive_buffer( uint8_t FUNCTION_ID )
{
	if( FUNCTION_ID_current == FUNCTION_ID ) return;
	FUNCTION_ID_current = FUNCTION_ID;
	if( receive_buffer != NULL )
	{
		free( receive_buffer );
		receive_buffer = NULL;
	}
	
	receive_buffer = ( uint8_t* )malloc( FUNCTION_ID );
	memset( receive_buffer, 0, FUNCTION_ID );
}








uint8_t* who_am_i( void )
{
	memory_allocation_receive_buffer( FUNCTION_ID_who_am_i );
	
	create_message( MPU9520_REGISTER_READ, MPU9520_WHO_AM_I, 0x00 );
	CS_ON();
	HAL_SPI_Transmit( &hspi, transmit_buffer, 1, HAL_MAX_DELAY );
	HAL_SPI_Receive( &hspi, receive_buffer, 1, HAL_MAX_DELAY );
	CS_OFF();
	
	return receive_buffer;
}




uint8_t* read_gyro( void )
{
	memory_allocation_receive_buffer( FUNCTION_ID_read_gyro );
	
	create_message( MPU9520_REGISTER_READ, MPU9520_GYRO_XOUT_L, 0x00 );
	CS_ON();
	HAL_SPI_Transmit( &hspi, transmit_buffer, 1, HAL_MAX_DELAY );
	HAL_SPI_Receive( &hspi, receive_buffer, 1, HAL_MAX_DELAY );
	CS_OFF();
	create_message( MPU9520_REGISTER_READ, MPU9520_GYRO_XOUT_H, 0x00 );
	CS_ON();
	HAL_SPI_Transmit( &hspi, transmit_buffer, 1, HAL_MAX_DELAY );
	HAL_SPI_Receive( &hspi, receive_buffer + 1, 1, HAL_MAX_DELAY );
	CS_OFF();
	
	
	create_message( MPU9520_REGISTER_READ, MPU9520_GYRO_YOUT_L, 0x00 );
	CS_ON();
	HAL_SPI_Transmit( &hspi, transmit_buffer, 1, HAL_MAX_DELAY );
	HAL_SPI_Receive( &hspi, receive_buffer + 2, 1, HAL_MAX_DELAY );
	CS_OFF();
	create_message( MPU9520_REGISTER_READ, MPU9520_GYRO_YOUT_H, 0x00 );
	CS_ON();
	HAL_SPI_Transmit( &hspi, transmit_buffer, 1, HAL_MAX_DELAY );
	HAL_SPI_Receive( &hspi, receive_buffer + 3, 1, HAL_MAX_DELAY );
	CS_OFF();
	
	
	create_message( MPU9520_REGISTER_READ, MPU9520_GYRO_ZOUT_L, 0x00 );
	CS_ON();
	HAL_SPI_Transmit( &hspi, transmit_buffer, 1, HAL_MAX_DELAY );
	HAL_SPI_Receive( &hspi, receive_buffer + 4, 1, HAL_MAX_DELAY );
	CS_OFF();
	create_message( MPU9520_REGISTER_READ, MPU9520_GYRO_ZOUT_H, 0x00 );
	CS_ON();
	HAL_SPI_Transmit( &hspi, transmit_buffer, 1, HAL_MAX_DELAY );
	HAL_SPI_Receive( &hspi, receive_buffer + 5, 1, HAL_MAX_DELAY );
	CS_OFF();
	
	return receive_buffer;
}




uint8_t* read_acc( void )
{
	memory_allocation_receive_buffer( FUNCTION_ID_read_acc );
	
	create_message( MPU9520_REGISTER_READ, MPU9520_ACCEL_XOUT_L, 0x00 );
	CS_ON();
	HAL_SPI_Transmit( &hspi, transmit_buffer, 1, HAL_MAX_DELAY );
	HAL_SPI_Receive( &hspi, receive_buffer, 1, HAL_MAX_DELAY );
	CS_OFF();
	create_message( MPU9520_REGISTER_READ, MPU9520_ACCEL_XOUT_H, 0x00 );
	CS_ON();
	HAL_SPI_Transmit( &hspi, transmit_buffer, 1, HAL_MAX_DELAY );
	HAL_SPI_Receive( &hspi, receive_buffer + 1, 1, HAL_MAX_DELAY );
	CS_OFF();
	
	
	create_message( MPU9520_REGISTER_READ, MPU9520_ACCEL_YOUT_L, 0x00 );
	CS_ON();
	HAL_SPI_Transmit( &hspi, transmit_buffer, 1, HAL_MAX_DELAY );
	HAL_SPI_Receive( &hspi, receive_buffer + 2, 1, HAL_MAX_DELAY );
	CS_OFF();
	create_message( MPU9520_REGISTER_READ, MPU9520_ACCEL_YOUT_H, 0x00 );
	CS_ON();
	HAL_SPI_Transmit( &hspi, transmit_buffer, 1, HAL_MAX_DELAY );
	HAL_SPI_Receive( &hspi, receive_buffer + 3, 1, HAL_MAX_DELAY );
	CS_OFF();
	
	
	create_message( MPU9520_REGISTER_READ, MPU9520_ACCEL_ZOUT_L, 0x00 );
	CS_ON();
	HAL_SPI_Transmit( &hspi, transmit_buffer, 1, HAL_MAX_DELAY );
	HAL_SPI_Receive( &hspi, receive_buffer + 4, 1, HAL_MAX_DELAY );
	CS_OFF();
	create_message( MPU9520_REGISTER_READ, MPU9520_ACCEL_ZOUT_H, 0x00 );
	CS_ON();
	HAL_SPI_Transmit( &hspi, transmit_buffer, 1, HAL_MAX_DELAY );
	HAL_SPI_Receive( &hspi, receive_buffer + 5, 1, HAL_MAX_DELAY );
	CS_OFF();
	
	return receive_buffer;
}


