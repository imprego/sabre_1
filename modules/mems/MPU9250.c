#include "mems/MPU9250.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>


typedef enum
{
	SUCCESS,
	ERROR
} mpu9520_result;



typedef enum
{
	MPU9520_SELF_TEST_X_GYRO =  	0x00,
	MPU9520_SELF_TEST_Y_GYRO =  	0x01,
	MPU9520_SELF_TEST_Z_GYRO =  	0x02,
	MPU9520_SELF_TEST_X_ACCEL = 	0x0D,
	MPU9520_SELF_TEST_Y_ACCEL = 	0x0E,
	MPU9520_SELF_TEST_Z_ACCEL = 	0x0F,
	MPU9520_XG_OFFSET_H =       	0x13,
	MPU9520_XG_OFFSET_L =       	0x14,
	MPU9520_YG_OFFSET_H =       	0x15,
	MPU9520_YG_OFFSET_L =       	0x16,
	MPU9520_ZG_OFFSET_H =       	0x17,
	MPU9520_ZG_OFFSET_L =       	0x18,
	MPU9520_SMPLRT_DIV =        	0x19,
	MPU9520_CONFIG =            	0x1A,
	MPU9520_GYRO_CONFIG =       	0x1B,
	MPU9520_ACCEL_CONFIG =      	0x1C,
	MPU9520_ACCEL_CONFIG_2 =    	0x1D,
	MPU9520_LP_ACCEL_ODR =      	0x1E,
	MPU9520_WOM_THR =           	0x1F,
	MPU9520_FIFO_EN =           	0x23,
	MPU9520_I2C_MST_CTRL =      	0x24,
	MPU9520_I2C_SLV0_ADDR =     	0x25,
	MPU9520_I2C_SLV0_REG =      	0x26,
	MPU9520_I2C_SLV0_CTRL =     	0x27,
	MPU9520_I2C_SLV1_ADDR =     	0x28,
	MPU9520_I2C_SLV1_REG =      	0x29,
	MPU9520_I2C_SLV1_CTRL =     	0x2A,
	MPU9520_I2C_SLV2_ADDR =     	0x2B,
	MPU9520_I2C_SLV2_REG =      	0x2C,
	MPU9520_I2C_SLV2_CTRL =     	0x2D,
	MPU9520_I2C_SLV3_ADDR =     	0x2E,
	MPU9520_I2C_SLV3_REG =      	0x2F,
	MPU9520_I2C_SLV3_CTRL =     	0x30,
	MPU9520_I2C_SLV4_ADDR =     	0x31,
	MPU9520_I2C_SLV4_REG =      	0x32,
	MPU9520_I2C_SLV4_DO =       	0x33,
	MPU9520_I2C_SLV4_CTRL =     	0x34,
	MPU9520_I2C_SLV4_DI =       	0x35,
	MPU9520_I2C_MST_STATUS =    	0x36,
	MPU9520_INT_PIN_CFG =       	0x37,
	MPU9520_INT_ENABLE =        	0x38,
	MPU9520_INT_STATUS =        	0x3A,
	MPU9520_ACCEL_XOUT_H =      	0x3B,
	MPU9520_ACCEL_XOUT_L =      	0x3C,
	MPU9520_ACCEL_YOUT_H =      	0x3D,
	MPU9520_ACCEL_YOUT_L =      	0x3E,
	MPU9520_ACCEL_ZOUT_H =      	0x3F,
	MPU9520_ACCEL_ZOUT_L =      	0x40,
	MPU9520_TEMP_OUT_H =        	0x41,
	MPU9520_TEMP_OUT_L =        	0x42,
	MPU9520_GYRO_XOUT_H =       	0x43,
	MPU9520_GYRO_XOUT_L =       	0x44,
	MPU9520_GYRO_YOUT_H =       	0x45,
	MPU9520_GYRO_YOUT_L =       	0x46,
	MPU9520_GYRO_ZOUT_H =       	0x47,
	MPU9520_GYRO_ZOUT_L =       	0x48,
	MPU9520_EXT_SENS_DATA_00 =  	0x49,
	MPU9520_EXT_SENS_DATA_01 =  	0x4A,
	MPU9520_EXT_SENS_DATA_02 =  	0x4B,
	MPU9520_EXT_SENS_DATA_03 =  	0x4C,
	MPU9520_EXT_SENS_DATA_04 =  	0x4D,
	MPU9520_EXT_SENS_DATA_05 =  	0x4E,
	MPU9520_EXT_SENS_DATA_06 =  	0x4F,
	MPU9520_EXT_SENS_DATA_07 =  	0x50,
	MPU9520_EXT_SENS_DATA_08 =  	0x51,
	MPU9520_EXT_SENS_DATA_09 =  	0x52,
	MPU9520_EXT_SENS_DATA_10 =  	0x53,
	MPU9520_EXT_SENS_DATA_11 =  	0x54,
	MPU9520_EXT_SENS_DATA_12 =  	0x55,
	MPU9520_EXT_SENS_DATA_13 =  	0x56,
	MPU9520_EXT_SENS_DATA_14 =  	0x57,
	MPU9520_EXT_SENS_DATA_15 =  	0x58,
	MPU9520_EXT_SENS_DATA_16 =  	0x59,
	MPU9520_EXT_SENS_DATA_17 =  	0x5A,
	MPU9520_EXT_SENS_DATA_18 =  	0x5B,
	MPU9520_EXT_SENS_DATA_19 =  	0x5C,
	MPU9520_EXT_SENS_DATA_20 =  	0x5D,
	MPU9520_EXT_SENS_DATA_21 =  	0x5E,
	MPU9520_EXT_SENS_DATA_22 =  	0x5F,
	MPU9520_EXT_SENS_DATA_23 =  	0x60,
	MPU9520_I2C_SLV0_DO =       	0x63,
	MPU9520_I2C_SLV1_DO =       	0x64,
	MPU9520_I2C_SLV2_DO =       	0x65,
	MPU9520_I2C_SLV3_DO =       	0x66,
	MPU9520_I2C_MST_DELAY_CTRL =	0x67,
	MPU9520_SIGNAL_PATH_RESET = 	0x68,
	MPU9520_MOT_DETECT_CTRL =   	0x69,
	MPU9520_USER_CTRL =         	0x6A,
	MPU9520_PWR_MGMT_1 =        	0x6B,
	MPU9520_PWR_MGMT_2 =        	0x6C,
	MPU9520_FIFO_COUNTH =       	0x72,
	MPU9520_FIFO_COUNTL =       	0x73,
	MPU9520_FIFO_R_W =          	0x74,
	MPU9520_WHO_AM_I =          	0x75,
	MPU9520_XA_OFFSET_H =       	0x77,
	MPU9520_XA_OFFSET_L =       	0x78,
	MPU9520_YA_OFFSET_H =       	0x7A,
	MPU9520_YA_OFFSET_L =       	0x7B,
	MPU9520_ZA_OFFSET_H =       	0x7D,
	MPU9520_ZA_OFFSET_L =       	0x7E
} mpu9520_register;


static uint8_t ( *send_receive )( uint16_t ) = NULL;


void set_send_receive_function( uint8_t ( *pf )( uint16_t ) )
{
	send_receive = pf;
}



static void mpu_register_write( mpu9520_register adress, uint8_t value )
{
	uint16_t send_data = ( adress << 4 ) + value;
	send_receive( send_data );
	return;
}


static uint8_t mpu_register_read( mpu9520_register adress )
{
	uint16_t send_data = 0x80 + ( adress << 4 );
	return send_receive( send_data );
}


void mpu_init( void )
{
	mpu_register_read( MPU9520_WHO_AM_I );
	
	
	return;
}




