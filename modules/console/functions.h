#ifndef __FUNCTIONS_H
#define __FUNCTIONS_H

#include "power/power.h"
#include "mems/mems.h"
#include "shock/shock.h"




void PING( int argc, char** argv )
{
	send( "PONG", strlen( "PONG" ), false );
	return;
}



void SETSTATE( int argc, char** argv )
{
	send( "NOT IMPLEMENTED", strlen( "NOT IMPLEMENTED" ), false );
	return;
}



void GETDATA( int argc, char** argv )
{
	if( argc > 4 || argc < 2 )
	{
		send( "ARGCERR", strlen( "ARGCERR" ), true );
		return;
	}
	
	bool mems_out = false, shock_out = false;
	bool mems_format_hex = true, shock_format_hex = true;
	
	for( uint8_t i = 1; i < argc; ++i )
	{
		if( strcmp( argv[ i ], "MEMSHEX" ) == 0 ) { mems_out = true; } //mems_format_hex = true;
		else if( strcmp( argv[ i ], "MEMSDEC" ) == 0 ) { mems_out = true; mems_format_hex = false; }
		else if( strcmp( argv[ i ], "SHOCKHEX" ) == 0 ) { shock_out = true; } //shock_format_hex = true;
		else if( strcmp( argv[ i ], "SHOCKDEC" ) == 0 ) { shock_out = true; shock_format_hex = false; }
	}
	
	
	/*
	if( mems_out & shock_out )
		setup_data_recv( mems | shock, mode_once );
	else if( mems_out )
		setup_data_recv( mems, mode_once );
	else if( shock_out )
		setup_data_recv( shock, mode_once );
	*/
	
	
	char answer[ 100 ] = { 0 };
	strcat( answer, argv[ 0 ] );
	char buf[ 50 ] = { 0 };
	
	if( mems_out )
	{
		int16_t temp[ 3 ];
		
		memset( temp, 0, 6 );
		get_mems_data( GYROSCOPE, ( uint8_t* )&temp );
		if( mems_format_hex )
			sprintf( buf, ",GX:%4hX,GY:%4hX,GZ:%4hX", *temp, *( temp + 1 ), *( temp + 2 ) );
		else
			sprintf( buf, ",GX:%6hd,GY:%6hd,GZ:%6hd", *temp, *( temp + 1 ), *( temp + 2 ) );
		strcat( answer, buf );
		
		memset( temp, 0, 6 );
		get_mems_data( ACCELEROMETER, ( uint8_t* )&temp );
		if( mems_format_hex )
			sprintf( buf, ",AX:%4hX,AY:%4hX,AZ:%4hX", *temp, *( temp + 1 ), *( temp + 2 ) );
		else
			sprintf( buf, ",AX:%6hd,AY:%6hd,AZ:%6hd", *temp, *( temp + 1 ), *( temp + 2 ) );
		strcat( answer, buf );
	}
	
	if( shock_out )
	{
		uint16_t temp = get_shock_data();
		if( shock_format_hex )
			sprintf( buf, ",SHOCK:%X", temp );
		else
			sprintf( buf, ",SHOCK:%d", temp );
		strcat( answer, buf );
	}
	
	send( answer, strlen( answer ), false );
	
	return;
}



void SHUTDOWN( int argc, char** argv )
{
	send( "POWER OFF", strlen( "POWER OFF" ), true );
	while( !__HAL_UART_GET_FLAG( &huart, UART_FLAG_TC ) );
	set_power( POWER_OFF );
	return;
}



void SETNUMBER( int argc, char** argv )
{
	send( "NOT IMPLEMENTED", strlen( "NOT IMPLEMENTED" ), true );
	return;
}

void GETSTATUS( int argc, char** argv )
{
	char answer[ 80 ] = { 0 };
	strcat( answer, argv[ 0 ] );
	
	for( uint8_t i = 1; i < argc; ++i )
	{
		if( strcmp( argv[i], "MEMS" ) == 0 )
		{
			strcat( answer, "," );
			if( mems_is_accessable() ) strcat( answer, "MEMS_AVAILABLE" );
			else strcat( answer, "MEMS_NOT_AVAILABLE" );
		}
		else if( strcmp( argv[i], "VERSION" ) == 0 )
		{
			extern const char* BUILD;
			strcat( answer, "," );
			strcat( answer, BUILD );
		}
	}
	
	send( answer, strlen( answer ), false );
	
	return;
}


void PRINTFIFO( int argc, char** argv )
{
	char answer[ 50 ] = { 0 };
	strcat( answer, argv[ 0 ] );
	strcat( answer, ",OK" );
	send( answer, strlen( answer ), false );
	
	print_fifo( true, "DEC", true, true );
	
	return;
}


void READREGISTER( int argc, char** argv )
{
	if( argc != 2 )
	{
		send( "ARGERR", strlen( "ARGERR" ), true );
	}
	
	char answer[ 50 ] = { 0 };
	char tmp[5] = { 0 };
	strcat( answer, argv[ 0 ] );
	strcat( answer, "," );
	
	tmp[ 0 ] = strlen( argv[ 1 ] );
	get_mems_data( READREG, ( uint8_t* )argv[ 1 ] );
	for( uint8_t i = 1; i < tmp[ 0 ]; ++i )
		*( argv[ 1 ] + i ) = 0x00;
	sprintf( tmp, "%2hX", *argv[ 1 ] );
	strncat( answer, tmp, 3 );
	
	send( answer, strlen( answer ), false );
	
	return;
}



const char *FUNCTIONS_NAMES[] = { "PING", "SETSTATE", "GETDATA", "SHUTDOWN", "SETNUMBER", "GETSTATUS", "PRINTFIFO", "READREGISTER", NULL };
void ( *FUNCTIONS_LIST[] )( int, char** ) = { PING, SETSTATE, GETDATA, SHUTDOWN, SETNUMBER, GETSTATUS, PRINTFIFO, READREGISTER };


#endif /* __FUNCTIONS_H */
