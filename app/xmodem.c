/*	
 * Copyright 2001-2010 Georges Menie (www.menie.org)
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the University of California, Berkeley nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE REGENTS AND CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* this code needs standard functions memcpy() and memset()
   and input/output functions _inbyte() and _outbyte().

   the prototypes of the input/output functions are:
     int _inbyte(unsigned short timeout); // msec timeout
     void _outbyte(int c);

 */
#include <ctype.h>
#include <stdio.h>		/* printf, scanf, NULL */
#include <stdlib.h>     /* malloc, free, rand */ 
#include <unistd.h>
#include <string.h>
#include "shell.h"
#include "crc16.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "sys.h"
#include "cmd.h"

#include "fs_port.h"
#include "AsyncIO/AsyncIO.h"
#include "AsyncIO/AsyncIOSerial.h"

#define SOH  0x01
#define STX  0x02
#define EOT  0x04
#define ACK  0x06
#define NAK  0x15
#define CAN  0x18
#define CTRLZ 0x1A

#define DLY_1S 1000
#define MAXRETRANS 25


static int xmodem_on=0;
static xQueueHandle xSerialRxQueue;
static char devname[DEVICE_NAME_LEN+1]="/dev/ttyUSB0";
static int iSerialReceive = 0;
//static xTaskHandle hSerialTask;
static int baudid=3;	// default 115200
//
// input character from uart
//
int _inbyte(int msec)
{
    int ch;
    // wait 10 ticks = 10ms?
    while ( pdFALSE == xQueueReceive( xSerialRxQueue, &ch, 10 ) )
    {
        msec-=10;
        if (msec <= 0)
			return -1;
    }
    printf("%s:%c(%x) ",__FUNCTION__, ch, ch);
    return ch;
}

//
// output character to uart
//
void _outbyte(unsigned char ch)
{
	printf("%s:%c(%x) ",__FUNCTION__, ch, ch);
    write(iSerialReceive, &ch, 1); // include crc
}


static int check(int crc, const unsigned char *buf, int sz)
{
	if (crc) {
		unsigned short crc = crc16_ccitt(buf, sz);
		unsigned short tcrc = (buf[sz]<<8)+buf[sz+1];
		if (crc == tcrc)
			return 1;
	}
	else {
		int i;
		unsigned char cks = 0;
		for (i = 0; i < sz; ++i) {
			cks += buf[i];
		}
		if (cks == buf[sz])
		return 1;
	}

	return 0;
}

static void flushinput(void)
{
	while (_inbyte(((DLY_1S)*3)>>1) >= 0)
		;
}

int xmodemReceive(int fd, int destsz)
{
	unsigned char xbuff[1030]; /* 1024 for XModem 1k + 3 head chars + 2 crc + nul */
	unsigned char *p;
	int bufsz, crc = 0;
	unsigned char trychar = 'C';
	unsigned char packetno = 1;
	int i, c, len = 0;
	int retry, retrans = MAXRETRANS;

	for(;;) {
		for( retry = 0; retry < 16; ++retry) {
			if (trychar) _outbyte(trychar);
			if ((c = _inbyte((DLY_1S)<<1)) >= 0) {
				switch (c) {
				case SOH:
					bufsz = 128;
					goto start_recv;
				case STX:
					bufsz = 1024;
					goto start_recv;
				case EOT:
					flushinput();
					_outbyte(ACK);
					return len; /* normal end */
				case CAN:
					if ((c = _inbyte(DLY_1S)) == CAN) {
						flushinput();
						_outbyte(ACK);
						return -1; /* canceled by remote */
					}
					break;
				default:
					break;
				}
			}
		}
		if (trychar == 'C') { trychar = NAK; continue; }
		flushinput();
		_outbyte(CAN);
		_outbyte(CAN);
		_outbyte(CAN);
		return -2; /* sync error */

	start_recv:
		if (trychar == 'C') crc = 1;
		trychar = 0;
		p = xbuff;
		*p++ = c;
		for (i = 0;  i < (bufsz+(crc?1:0)+3); ++i) {
			if ((c = _inbyte(DLY_1S)) < 0) goto reject;
			*p++ = c;
		}

		if (xbuff[1] == (unsigned char)(~xbuff[2]) && 
			(xbuff[1] == packetno || xbuff[1] == (unsigned char)packetno-1) &&
			check(crc, &xbuff[3], bufsz)) {
			if (xbuff[1] == packetno)	{
				register int count = destsz - len;
				if (count > bufsz) count = bufsz;
				if (count > 0) {
					if (fs_write(fd, &xbuff[3], count)!=count)
					{
						printf("write error\n");
						goto reject;
					}
					//memcpy (&dest[len], &xbuff[3], count);
					len += count;
				}
				++packetno;
				retrans = MAXRETRANS+1;
			}
			if (--retrans <= 0) {
				flushinput();
				_outbyte(CAN);
				_outbyte(CAN);
				_outbyte(CAN);
				return -3; /* too many retry error */
			}
			_outbyte(ACK);
			continue;
		}
	reject:
		flushinput();
		_outbyte(NAK);
	}
}

int xmodemTransmit(int fd, int srcsz)
{
	unsigned char xbuff[1030]; /* 1024 for XModem 1k + 3 head chars + 2 crc + nul */
	int bufsz, crc = -1;
	unsigned char packetno = 1;
	int i, c, len = 0;
	int retry;

	for(;;) {
		for( retry = 0; retry < 16; ++retry) {
			if ((c = _inbyte((DLY_1S)<<1)) >= 0) {
				switch (c) {
				case 'C':
					crc = 1;
					goto start_trans;
				case NAK:
					crc = 0;
					goto start_trans;
				case CAN:
					if ((c = _inbyte(DLY_1S)) == CAN) {
						_outbyte(ACK);
						flushinput();
						return -1; /* canceled by remote */
					}
					break;
				default:
					break;
				}
			}
		}
		_outbyte(CAN);
		_outbyte(CAN);
		_outbyte(CAN);
		flushinput();
		return -2; /* no sync */

		for(;;) {
		start_trans:
			xbuff[0] = SOH; bufsz = 128;
			xbuff[1] = packetno;
			xbuff[2] = ~packetno;
			c = srcsz - len;
			if (c > bufsz) c = bufsz;
			if (c >= 0) {
				memset (&xbuff[3], 0, bufsz);
				if (c == 0) {
					xbuff[3] = CTRLZ;
				}
				else {
					if (fs_read(fd, &xbuff[3], c) != c)
					{
						printf("read error\n");
						_outbyte(NAK);
						flushinput();
						return -5; /* canceled by remote */
					}
					//memcpy (&xbuff[3], &src[len], c);
					if (c < bufsz) xbuff[3+c] = CTRLZ;
				}
				if (crc) {
					unsigned short ccrc = crc16_ccitt(&xbuff[3], bufsz);
					xbuff[bufsz+3] = (ccrc>>8) & 0xFF;
					xbuff[bufsz+4] = ccrc & 0xFF;
				}
				else {
					unsigned char ccks = 0;
					for (i = 3; i < bufsz+3; ++i) {
						ccks += xbuff[i];
					}
					xbuff[bufsz+3] = ccks;
				}
				for (retry = 0; retry < MAXRETRANS; ++retry) {
					for (i = 0; i < bufsz+4+(crc?1:0); ++i) {
						_outbyte(xbuff[i]);
					}
					if ((c = _inbyte(DLY_1S)) >= 0 ) {
						switch (c) {
						case ACK:
							++packetno;
							len += bufsz;
							goto start_trans;
						case CAN:
							if ((c = _inbyte(DLY_1S)) == CAN) {
								_outbyte(ACK);
								flushinput();
								return -1; /* canceled by remote */
							}
							break;
						case NAK:
						default:
							break;
						}
					}
				}
				_outbyte(CAN);
				_outbyte(CAN);
				_outbyte(CAN);
				flushinput();
				return -4; /* xmit error */
			}
			else {
				for (retry = 0; retry < 10; ++retry) {
					_outbyte(EOT);
					if ((c = _inbyte((DLY_1S)<<1)) == ACK) break;
				}
				flushinput();
				return (c == ACK)?len:-5;
			}
		}
	}
}


//
// enable xmodemint xmodem_on
//
void cmd_xmodem(int argc, char* argv[])
{
    int i,c, ret=0, op=-1, fd=-1;
    static char filename[65]="temp";
    FILINFO stat;
    
    while ((c = getopt (argc, argv, "d:b:f:rwh?")) != -1)
    {
	    switch (c) {
		case 'd':
			strncpy(devname,optarg,DEVICE_NAME_LEN);
		break;
		case 'b':
			for(i=0; i<MAX_BAUD_NUM; i++)
            {
                if (strcmp(optarg,baudstr[i])==0)
                {
                    baudid = i;
                    printf("set baudrate %s\n",baudstr[baudid]);
                    break;
                }
            }
		break;
    	case 'r':
    	    op = 0;
	    break;
	    case 'w':
	        op = 1;
	    break;
	    case 'f':
	        strncpy(filename,optarg,64);
	    break;
	    case '?':
	    case 'h':
	    default:
	        printf("usage: -r(read data from console) | -w (write data to console) -f<file>\n"
				"-d <device name> -b <baudrate>\n");
	        goto end_xmodem;
	    }
    }   // end of while
    
    if (op == -1)
    {
        printf("Xmodem setting: device %s, baud %s, mode=%s, file=%s\n",
			devname, baudstr[baudid], (op?"read":"write"), filename);
        goto end_xmodem;
    }
    
    if (xmodem_on)
    {
        printf("serial port inuse\n");
        goto end_xmodem;
    }
    // open file for read/write
    fd = fs_open(filename, op ? FA_READ : (FA_CREATE_NEW| FA_WRITE)); 
    if (fd < 0) 
    {
		printf("xmodem: fopen()");
		goto end_xmodem;
    }
    
    // open uart device
    if ( pdTRUE == lAsyncIOSerialOpen( devname, &iSerialReceive,  baudrate[baudid]) )
    {
        xSerialRxQueue = xQueueCreate( MAX_PKT_SIZE*2, sizeof ( unsigned char ) );
        (void)lAsyncIORegisterCallback( iSerialReceive, vAsyncSerialIODataAvailableISR,
                                        xSerialRxQueue );
        printf("Open device %s success\n",devname);
        
        xmodem_on = 1;
        
        if (op==0)
        {    
            ret = xmodemReceive(fd, 0x100000);	// restrict max size 1MB
            if (ret < 0)
		        printf("Xmodem receive error: status: %d\n", ret);
	        else  
		        printf("Xmodem successfully received data len=%d bytes\n", ret);
        }
        else if (op==1)
        {
			// get file size
			if (fs_stat(filename,&stat)==FR_OK)
			{
				ret = xmodemTransmit(fd, stat.fsize);
				if (ret < 0)
					printf("Xmodem transmit error: status: %d\n", ret);
				else  
					printf("Xmodem successfully transmitted %d bytes\n", ret);
			}
			else
				printf("get file size error\n");
		}  
        // close device
        lAsyncIOSerialClose(iSerialReceive);
        iSerialReceive = 0;
        vQueueDelete(xSerialRxQueue);
        xmodem_on = 0;
    }   
end_xmodem:	
    if (fd>0)
		fs_close(fd);
    return;
}



#ifdef TEST_XMODEM_RECEIVE
int main(void)
{
	int st;

	printf ("Send data using the xmodem protocol from your terminal emulator now...\n");
	/* the following should be changed for your environment:
	   0x30000 is the download address,
	   65536 is the maximum size to be written at this address
	 */
	st = xmodemReceive((char *)0x30000, 65536);
	if (st < 0) {
		printf ("Xmodem receive error: status: %d\n", st);
	}
	else  {
		printf ("Xmodem successfully received %d bytes\n", st);
	}

	return 0;
}
#endif
#ifdef TEST_XMODEM_SEND
int main(void)
{
	int st;

	printf ("Prepare your terminal emulator to receive data now...\n");
	/* the following should be changed for your environment:
	   0x30000 is the download address,
	   12000 is the maximum size to be send from this address
	 */
	st = xmodemTransmit((char *)0x30000, 12000);
	if (st < 0) {
		printf ("Xmodem transmit error: status: %d\n", st);
	}
	else  {
		printf ("Xmodem successfully transmitted %d bytes\n", st);
	}

	return 0;
}
#endif