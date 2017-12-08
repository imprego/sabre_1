#ifndef __INDICATION_H
#define __INDICATION_H

typedef enum
{
	INDICATION_ON,
	INDICATION_OFF
} indication_state;

typedef enum
{
	INDICATION_RED,
	INDICATION_GREEN
} indication_color;


void indication_init( void );
void set_indication( indication_color color, indication_state state );

#endif /* __INDICATION_H */
