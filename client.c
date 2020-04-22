/*
 * client.c
 *
 * Copyright(C) 2006 Lee Chang-woo <hl2irw@armstudy.org>
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>
#include <curses.h>

#define DEBUG	
#ifdef DEBUG
#define DPRINTF(fmt, args...)	fprintf(stderr,"DEBUG: " __FILE__ ": " fmt, ##args)
#else
#define DPRINTF(fmt, args...)	do { } while(0);
#endif

#define SERVER_IP	"10.50.3.191"
#define SERVER_PORT	9740

#define TIMESET		1
#define BUF_SIZE 255


int mygetch(void)
{
	struct termios oldt, newt;
	int ch;
	tcgetattr( STDIN_FILENO, &oldt);
	newt = oldt;
	newt.c_lflag &= !(ICANON | ECHO);
	tcsetattr( STDIN_FILENO, TCSANOW, &newt);
	ch = getchar();
	tcsetattr( STDIN_FILENO, TCSANOW, &oldt);
	return ch;
}

char *itoa(int val, int base)
{
    static char buf[32] = {0};
    int i = 30;
    for(; val && i ; --i, val /= base)
        buf[i] = "0123456789abcdef"[val % base];
    return &buf[i+1];
}
 

int main (void)
{
    int server;
    int len = 0; 
    struct sockaddr_in server_addr;	/* AF_INET */
    char buf[255];
    int recv_len;
    int run_enable;
    struct timeval old_tv, cur_tv;
    int retry = 0;
    int time_ck,time_check,res;
	int str_len;
	char key_input;	// 키보드 입력값
	int sendData;
	
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("10.50.3.191");
    server_addr.sin_port = htons(9740);
    len = sizeof(server_addr);			    		  
    run_enable = 1;

    /* server connection start */
              /* create socket */
    server = socket(PF_INET, SOCK_STREAM, 0);
    res = connect(server, (struct sockaddr *)&server_addr,len);
    if (res < 0) {
        fprintf(stderr, "client: can't connect (socket)\n");
	exit(1);
    }
	retry = 0;
	printf("read data \n");
	str_len = read(server, buf, BUF_SIZE);
	sendData = atoi(buf);
	printf("read data : %s, %d\n", buf, sendData);

	
	while( key_input != 3)
	{
		key_input = mygetch();
		printf("입력된 키 : %d\n", key_input);
		buf = itoa( recv_data, 10 );
		if( key_input == 'w')
		{
			buf = itoa( recv_data+20 , 10 );
			write(server,buf,3); 
		}
		else if( key_input == 's' )
		{
			buf = itoa( recv_data+10 , 10 );
			write(server, buf, 3);
		}
		usleep(10000);
	}
  
	       
	
      
    close(server);
    return 0;
}
