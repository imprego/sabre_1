#ifndef __CONSOLE_H
#define __CONSOLE_H

#include <stdbool.h>

void console_init( void );
bool is_send( void );
void send( char* data, unsigned short len, bool raw );


#endif /* __CONSOLE_H */
