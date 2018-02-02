#ifndef __FUNCTIONS_H
#define __FUNCTIONS_H

#include "power/power.h"
#include "mems/mems.h"
#include "shock/shock.h"




void PING( int argc, char** argv )
{
	send( "PONG", strlen( "PONG" ) );
	return;
}



void SETSTATE( int argc, char** argv )
{
	send( "RESERVED", strlen( "RESERVED" ) );
	return;
}



void GETDATA( int argc, char** argv )
{
	if( argc > 5 || argc < 2 )
	{
		send( "ARGCERR", strlen( "ARGCERR" ) );
		return;
	}
	
	bool mems_out = false, shock_out = false;
	bool mems_mode_once = true, shock_mode_once = true;
	bool mems_format_hex = true, shock_format_hex = true;
	char first_param = 'n';
	char current_param = 'n';
	
	for( uint8_t i = 1; i < argc; ++i )
	{
		if( strcmp( argv[ i ], "MEMS" ) == 0 ) { mems_out = true; current_param = 'm'; }
		else if( strcmp( argv[ i ], "SHOCK" ) == 0 ) { shock_out = true;  current_param = 's'; }
		else if( strcmp( argv[ i ], "ONCE" ) == 0 ) ;// default parameter value
		else if( strcmp( argv[ i ], "CONT" ) == 0 )
		{
			#warning "CONTINUE"
			continue;
			if( current_param == 'm' ) mems_mode_once = false;
			else if( current_param == 's' ) shock_mode_once = false;
		}
		else if( strcmp( argv[ i ], "HEX" ) == 0 ) ;// default parameter value
		else if( strcmp( argv[ i ], "DEC" ) == 0 )
		{
			if( current_param == 'm' ) mems_format_hex = false;
			else if( current_param == 's' ) shock_format_hex = false;
		}
		
		if( first_param == 'n' && current_param != 'n' ) first_param = current_param;
	}
	
	
	char answer[ 100 ] = { 0 };
	strcat( answer, argv[ 0 ] );
	char buf[ 50 ] = { 0 };
	
	while( mems_out || shock_out )
	{
		if( mems_out && first_param == 'm' )
		{
			int16_t temp[ 3 ];
			
			temp[ 0 ] = temp[ 1 ] = temp[ 2 ] = 0x0000;
			mems_out = false;
			get_mems_data( GYROSCOPE, ( uint8_t* )&temp );
			if( mems_format_hex )
				sprintf( buf, ",GYRO:%4hX:%4hX:%4hX", *temp, *( temp + 1 ), *( temp + 2 ) );
			else
				sprintf( buf, ",GYRO:%6hd:%6hd:%6hd", *temp, *( temp + 1 ), *( temp + 2 ) );
			strcat( answer, buf );
			
			
			temp[ 0 ] = temp[ 1 ] = temp[ 2 ] = 0x0000;
			get_mems_data( ACCELEROMETER, ( uint8_t* )&temp );
			if( mems_format_hex )
				sprintf( buf, ",ACC:%4hX:%4hX:%4hX", *temp, *( temp + 1 ), *( temp + 2 ) );
			else
				sprintf( buf, ",ACC:%6hd:%6hd:%6hd", *temp, *( temp + 1 ), *( temp + 2 ) );
			strcat( answer, buf );
			first_param = 's';
		}
		if( shock_out && first_param == 's' )
		{
			shock_out = false;
			uint16_t temp = get_shock_data();
			if( shock_format_hex )
				sprintf( buf, ",SHOCK:%X", temp );
			else
				sprintf( buf, ",SHOCK:%d", temp );
			strcat( answer, buf );
			first_param = 'm';
		}
	}
	
	send( answer, strlen( answer ) );
	
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
	send( "RESERVED", strlen( "RESERVED" ) );
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
			uint8_t temp = 0x00;
			get_mems_data( WHOAMI, ( uint8_t* )&temp );
			strcat( answer, "," );
			if( temp == 0x71 ) strcat( answer, "MEMS_AVAILABLE" );
			else strcat( answer, "MEMS_NOT_AVAILABLE" );
		}
		if( strcmp( argv[i], "VERSION" ) == 0 )
		{
			extern const char* BUILD;
			strcat( answer, "," );
			strcat( answer, BUILD );
		}
	}
	
	send( answer, strlen( answer ) );
	
	return;
}



const char *FUNCTIONS_NAMES[] = { "PING", "SETSTATE", "GETDATA", "SHUTDOWN", "SETNUMBER", "GETSTATUS", NULL };
void ( *FUNCTIONS_LIST[] )( int, char** ) = { PING, SETSTATE, GETDATA, SHUTDOWN, SETNUMBER, GETSTATUS };


#endif /* __FUNCTIONS_H */
