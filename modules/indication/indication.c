#include "indication/indication.h"
#include "stm32f4xx_hal.h"


#define PIN_RED			GPIO_PIN_8
#define PORT_RED		GPIOB
#define PIN_GREEN		GPIO_PIN_9
#define PORT_GREEN	GPIOB


void indication_init( void )
{
	__HAL_RCC_GPIOB_CLK_ENABLE();
	
	GPIO_InitTypeDef gpio;
	//gpio.Alternate = ;
	gpio.Mode = GPIO_MODE_OUTPUT_PP;
	gpio.Pin = PIN_RED;
	gpio.Pull = GPIO_NOPULL;
	gpio.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init( PORT_RED, &gpio );
	HAL_GPIO_WritePin( PORT_RED, PIN_RED, GPIO_PIN_RESET );
	
	gpio.Pin = PIN_GREEN;
	HAL_GPIO_Init( PORT_GREEN, &gpio );
	HAL_GPIO_WritePin( PORT_GREEN, PIN_GREEN, GPIO_PIN_RESET );
	
	return;
}


void set_indication( indication_color color, indication_state state )
{
	GPIO_PinState pin_state;
	if( state == INDICATION_ON )
		pin_state = GPIO_PIN_RESET;
	else if( state == INDICATION_OFF )
		pin_state = GPIO_PIN_SET;
	
	
	switch( color )
	{
		case INDICATION_RED:
			HAL_GPIO_WritePin( PORT_RED, PIN_RED, pin_state );
			break;
		case INDICATION_GREEN:
			HAL_GPIO_WritePin( PORT_GREEN, PIN_GREEN, pin_state );
			break;
	}
	return;
}
