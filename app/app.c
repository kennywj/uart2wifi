//
//porting of fatfs
//
#include <stdio.h> 
#include <stdlib.h>     /* malloc, free, rand */
#include <string.h>     /* memcpy */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

extern int tcpecho_server_init(void);
extern int http_server_init(void);
extern int tftp_server_init(void);

void app_init()
{
	tcpecho_server_init();
	
	http_server_init();
	
	tftp_server_init();
}
