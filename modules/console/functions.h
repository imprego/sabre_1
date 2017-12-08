#ifndef __FUNCTIONS_H
#define __FUNCTIONS_H

#include "power/power.h"



void PING( int argc, char** argv )
{
	send( "PONG", strlen( "PONG" ) );
	return;
}

void SETSTATE( int argc, char** argv )
{
	return;
}


void GETDATA( int argc, char** argv )
{
	return;
}


void GETVERSION( int argc, char** argv )
{
	return;
}


void SHUTDOWN( int argc, char** argv )
{
	send( "POWER OFF", strlen( "POWER OFF" ) );
	while( !__HAL_UART_GET_FLAG( &huart, UART_FLAG_TC ) );
	set_power( POWER_OFF );
	return;
}


void SETNUMBER( int argc, char** argv )
{
	return;
}


const char *FUNCTIONS_NAMES[] = { "PING", "SETSTATE", "GETDATA", "GETVERSION", "SHUTDOWN", "SETNUMBER", NULL };
void ( *FUNCTIONS_LIST[] )( int, char** ) = { PING, SETSTATE, GETDATA, GETVERSION, SHUTDOWN, SETNUMBER };


#endif /* __FUNCTIONS_H */
