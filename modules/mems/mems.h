#ifndef __MEMS_H
#define __MEMS_H

#include <stdbool.h>
#include <stdint.h>

void vMemsInitProc( void* vParams );
void vMemsWorkProc( void* vParams );
void vMemsSyncProc( void* vParams );

bool mems_is_accessable( void );
void get_acc_gyro_data( uint8_t* recv_buf );
void flush_fifo_buf( void );
uint16_t** get_acc_gyro_buf( void );

#endif /* __MEMS_H */
