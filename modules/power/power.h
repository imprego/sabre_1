#ifndef __POWER_H
#define __POWER_H

typedef enum
{
	POWER_ON = 0x01,
	POWER_OFF = 0x02
} power_state;

void power_init( void );
void set_power( power_state state );

#endif /* __POWER_H */
