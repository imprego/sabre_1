#include "stm32f4xx_hal.h"

#include "power/power.h"
#include "mems/mems.h"
#include "shock/shock.h"

#include "FreeRTOS.h"
#include "task.h"

#include "console/console.h"
#include <string.h>
#include <stdio.h>




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
	char mems_out_mode = 'n', shock_out_mode = 'n', output_mode = 'o';
	
	for( uint8_t i = 1; i < argc; ++i )
	{
		if( strcmp( argv[ i ], "MEMSHEX" ) == 0 ) mems_out_mode = 'h';
		else if( strcmp( argv[ i ], "MEMSDEC" ) == 0 ) mems_out_mode = 'd';
		else if( strcmp( argv[ i ], "SHOCKHEX" ) == 0 ) shock_out_mode = 'h';
		else if( strcmp( argv[ i ], "SHOCKDEC" ) == 0 ) shock_out_mode = 'd';
		else if( strcmp( argv[ i ], "ONCE" ) == 0 ) ; //output_mode = 'o';
		else if( strcmp( argv[ i ], "BUFF" ) == 0 ) output_mode = 'b';
	}
	
	char answer[ 100 ] = { 0 };
	strcat( answer, argv[ 0 ] );
	char buf[ 80 ] = { 0 };
	
	if( output_mode == 'o' )
	{
		if( mems_out_mode != 'n' )
		{
			uint16_t tmp_mems[ 6 ] = { 0 };
			
			get_acc_gyro_data( ( uint8_t* )&tmp_mems );
			
			if( mems_out_mode == 'h' )
				sprintf( buf, ",AX:%4hX,AY:%4hX,AZ:%4hX,GX:%4hX,GY:%4hX,GZ:%4hX",
									*tmp_mems, *( tmp_mems + 1 ), *( tmp_mems + 2 ),
									*( tmp_mems + 3 ), *( tmp_mems + 4 ), *( tmp_mems + 5 ) );
			else if( mems_out_mode == 'd' )
				sprintf( buf, ",AX:%6hd,AY:%6hd,AZ:%6hd,GX:%6hd,GY:%6hd,GZ:%6hd",
									*tmp_mems, *( tmp_mems + 1 ), *( tmp_mems + 2 ),
									*( tmp_mems + 3 ), *( tmp_mems + 4 ), *( tmp_mems + 5 ) );
			strcat( answer, buf );
		}
		
		if( shock_out_mode != 'n' )
		{
			uint16_t tmp_shock = get_shock_data();
			if( shock_out_mode == 'h' )
				sprintf( buf, ",SHOCK:%4hX", tmp_shock );
			else if( shock_out_mode == 'd' )
				sprintf( buf, ",SHOCK:%6hd", tmp_shock );
			strcat( answer, buf );
		}
		
		send( answer, strlen( answer ), false );
	}
	else if( output_mode == 'b' )
	{
		send( answer, strlen( answer ), false );
		strcpy( answer, "" );
		
		taskENTER_CRITICAL();
		uint16_t* tmp_mems_buf[ 4 ];
		for( uint8_t i = 0; i < 4; ++i )
			tmp_mems_buf[ i ] = get_acc_gyro_buf()[ i ];
		
		uint16_t* tmp_shock_buf[ 4 ];
		for( uint8_t i = 0; i < 4; ++i )
			tmp_shock_buf[ i ] = get_shock_buf()[ i ];
		taskEXIT_CRITICAL();
		
		for( uint8_t i = 0; i < 3; ++i ) 
				for( uint8_t j = 0; j < 25; ++j )
				{
					if( mems_out_mode != 'n' )
					{
						if( mems_out_mode == 'h' )
							sprintf( buf, "AX:%4hX,AY:%4hX,AZ:%4hX,GX:%4hX,GY:%4hX,GZ:%4hX",
												tmp_mems_buf[ i ][ j * 6 ], tmp_mems_buf[ i ][ j * 6 + 1 ], tmp_mems_buf[ i ][ j * 6 + 2 ],
												tmp_mems_buf[ i ][ j * 6 + 3 ], tmp_mems_buf[ i ][ j * 6 + 4 ], tmp_mems_buf[ i ][ j * 6 + 5 ] );
						else if( mems_out_mode == 'd' )
							sprintf( buf, "AX:%6hd,AY:%6hd,AZ:%6hd,GX:%6hd,GY:%6hd,GZ:%6hd",
												tmp_mems_buf[ i ][ j * 6 ], tmp_mems_buf[ i ][ j * 6 + 1 ], tmp_mems_buf[ i ][ j * 6 + 2 ],
												tmp_mems_buf[ i ][ j * 6 + 3 ], tmp_mems_buf[ i ][ j * 6 + 4 ], tmp_mems_buf[ i ][ j * 6 + 5 ] );
						strcat( answer, buf );
						if( shock_out_mode != 'n' ) strcat( answer, "," );
					}
					if( shock_out_mode != 'n' )
					{
						if( shock_out_mode == 'h' )
							sprintf( buf, "SHOCK:%4hX", tmp_shock_buf[ i ][ j ] );
						else if( shock_out_mode == 'd' )
							sprintf( buf, "SHOCK:%6hd", tmp_shock_buf[ i ][ j ] );
						strcat( answer, buf );
					}
					send( answer, strlen( answer ), true );
					strcpy( answer, "" );
				}
	}
	
	return;
}



void SHUTDOWN( int argc, char** argv )
{
	send( "POWER OFF", strlen( "POWER OFF" ), true );
	//while( !__HAL_UART_GET_FLAG( &huart, UART_FLAG_TC ) );
	vTaskDelay( 25 );
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




const char *FUNCTIONS_NAMES[] = { "PING", "SETSTATE", "GETDATA", "SHUTDOWN", "SETNUMBER", "GETSTATUS", NULL };
void ( *FUNCTIONS_LIST[] )( int, char** ) = { PING, SETSTATE, GETDATA, SHUTDOWN, SETNUMBER, GETSTATUS };
