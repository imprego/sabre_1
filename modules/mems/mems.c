#include "stm32f4xx_hal.h"
#include "mems/mems.h"
#include "mems/MPU9250.h"



static void send						( char* data, unsigned short len );



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
	
	
	__HAL_RCC_SPI1_CLK_ENABLE();
	SPI_HandleTypeDef hspi;
	hspi.Instance = SPI2;
	hspi.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_64;
	hspi.Init.CLKPhase = SPI_PHASE_2EDGE;
	hspi.Init.CLKPolarity = SPI_POLARITY_HIGH;
	hspi.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
	hspi.Init.CRCPolynomial = 7;
	hspi.Init.DataSize = SPI_DATASIZE_16BIT;
	hspi.Init.Direction = SPI_DIRECTION_2LINES;
	hspi.Init.FirstBit = SPI_FIRSTBIT_MSB;
	hspi.Init.Mode = SPI_MODE_MASTER;
	hspi.Init.NSS = SPI_NSS_HARD_OUTPUT;
	hspi.Init.TIMode = SPI_TIMODE_DISABLE;
	HAL_SPI_Init( &hspi );
	
	
	return;
}


static void send_receive( char* data, unsigned short len )
{
	HAL_GPIO_WritePin( GPIOB, GPIO_PIN_12, GPIO_PIN_RESET );
	//spi send
	//spi_receive
	HAL_GPIO_WritePin( GPIOB, GPIO_PIN_12, GPIO_PIN_SET );
	
	
	return;
}

