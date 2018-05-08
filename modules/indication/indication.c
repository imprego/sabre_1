#include "indication/indication.h"
#include "stm32f4xx_hal.h"

extern void _Error_Handler( char * file, int line );


//PIN_RED			GPIO_PIN_8
//PORT_RED		GPIOB
//PIN_GREEN		GPIO_PIN_9
//PORT_GREEN	GPIOB

void indication_init( void )
{
	__HAL_RCC_GPIOB_CLK_ENABLE();
	
	GPIO_InitTypeDef gpio;
	gpio.Mode = GPIO_MODE_OUTPUT_PP;
	gpio.Pin = GPIO_PIN_8 | GPIO_PIN_9;
	gpio.Pull = GPIO_NOPULL;
	gpio.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init( GPIOB, &gpio );
	
	set_indication( INDICATION_LED_ALL, INDICATION_STATE_OFF );
	
	return;
}


void set_indication( indication_led led, indication_state state )
{
	GPIO_PinState pin_state;
	if( state == INDICATION_STATE_ON )
		pin_state = GPIO_PIN_RESET;
	else if( state == INDICATION_STATE_OFF )
		pin_state = GPIO_PIN_SET;
	
	switch( led )
	{
		case INDICATION_LED_RED:
			HAL_GPIO_WritePin( GPIOB, GPIO_PIN_8, pin_state );
			break;
		case INDICATION_LED_GREEN:
			HAL_GPIO_WritePin( GPIOB, GPIO_PIN_9, pin_state );
			break;
		case INDICATION_LED_ALL:
			HAL_GPIO_WritePin( GPIOB, GPIO_PIN_8 | GPIO_PIN_9, pin_state );
			break;
	}
	
	return;
}
