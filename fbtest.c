/*
    fbtest.c  framebuffer test    hl2irw@armstudy.org
*/
#include <sys/time.h>
#include <sys/types.h>
#include <asm/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>
#include <linux/fs.h>
#include <linux/rtc.h>
#include <linux/fb.h>
#include <pthread.h>
#include <stdarg.h>
#include <termios.h>
#include "color.h"
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <pthread.h>

#define FB_DEV_NODE 		"/dev/fb0"
#define XSIZE           	800
#define YSIZE			480
#define disp_offset		0
#define BUF_SIZE 3 
#define EPOLL_SIZE 50
#define PORT 9740

#define MIN(a,b)		((a) < (b) ? (a) : (b))
#define MAX(a,b)		((a) > (b) ? (a) : (b))
#define X_DEFFERENT 2
#define Y_DEFFERENT 70


void game_Stadium();
void ball();
void setnonblockingmode(int fd);
void error_handling(char *buf);
static void *threadFunc(void *arg);
void strcmp_(char *data);

int move_x=400, move_y=200;
int upstate=0, rightstate = 0;
int p1_x = 130, p1_y = 200, p2_x = 670 , p2_y = 200 ;
int sleep_Time=5000;

struct Bit_Map_Header
{
      unsigned short bfType;       // 2Byte  항상 'BM'
      unsigned long  bfSize;       // file size               전체 파일 크기
      unsigned short bfReserved1;  // 2Byte integer, 항상
      unsigned short bfReserved2;  // 2Byte integer, 항상
      unsigned long  bfOffBits;    // Offset Long 형          실제 이미지 데이터 오프셋
      long biSize;                 // long형                  비트맵 정보 헤더의 크기 
      long biWidth;                // long형, Pixel 단위      이미지의 X 길이
      long biHeight;               // long형, Pixel 단위      이미지의 Y 길이
      short biPlanes;              // integer형, 보통 1       비트 평면 수
      short biBitCount;            // integer형               Pixel당 비트 수
      long biCompression;          // long형                  압축형태 
      long biSizeImage;            // long형                  압축되지 않은 이미지의 총 바이트 수 
      long biXpelsPerMeter;        // long형                  출력 디바이스의 수평 해상도
      long biYpelsPerMeter;        // long형                  출력 디바이스의 수직 해상도 
      long biClrUsed;              // long형                  사용 칼라 수
      long biClrImportant;         // long형                  중요한 칼라 수
} *Bit_Map;

#define BI_RGB          0    // 압축되지 않은 비트맵
#define BI_RLE8         1    // 셀당 8비트 비트맵의 Run Length 압축
#define BI_RLE4         2    // 픽셀당 4비트 비트맵의 Run Length 압축 

/*
 *    biSizeImage = biWidth * biHeight       // 256칼라일 경우
 *    biSizeImage = biWidth * biHeight * 3   // 24bit 칼라일경우
 *    if ((biWidth % 4) != 0) {
 *       ImageSize = biWidth * biHeight * 3 + biHeight * (biWidth % 4);
 *    } else {
 *       ImageSize = biWidth * biHeight * 3;
 *    }       
 * 
 */

int sys_color,maxx,maxy;
int fb_fd = -1;
int fb_length = 0;
unsigned short *fbmem = NULL;
unsigned short (*frameBuffer)[XSIZE];
unsigned short fg_color,bg_color;
int old_sec;
struct tm *tr;
time_t ntime;
int dg[15];
int clipminx, clipminy, clipmaxx, clipmaxy;



char *itoa(int val, int base)
{
    static char buf[32] = {0};
    int i = 30;
    for(; val && i ; --i, val /= base)
        buf[i] = "0123456789abcdef"[val % base];
    return &buf[i+1];
}
 

void init_data (void)
{
      maxx = XSIZE;
      maxy = YSIZE;
      clipminx = 0;
      clipminy = 0;
      clipmaxx = maxx - 1;
      clipmaxy = maxy - 1;
}


void SetPixel (int x, int y, unsigned short color)
{
      if ((x >= maxx) || (y >= maxy)) return;
      if ((x < 0) || (y < 0)) return;  
      frameBuffer[y][x] = color;
}


unsigned short GetPixel (int x, int y)
{
      return frameBuffer[y][x];
}


int *fb_memcpy (unsigned int *dst, unsigned int *src, unsigned int count)
{
      while (count--) {
     	    *dst = *src;
    	    dst++;
   	    src++;
      }
      return 0;
}


unsigned int fb_grab (int fd, unsigned short **fbmem)
{
      struct fb_var_screeninfo modeinfo;
      unsigned int length;
      if (ioctl(fd, FBIOGET_VSCREENINFO, &modeinfo) < 0) {
         perror("FBIOGET_VSCREENINFO");
         exit (EXIT_FAILURE);
      }
      length = modeinfo.xres * modeinfo.yres * (modeinfo.bits_per_pixel >> 3);
      printf("  %d x %d, %d bpp\n\r",modeinfo.xres, modeinfo.yres, modeinfo.bits_per_pixel);
      maxx = modeinfo.xres;
      maxy = modeinfo.yres;
      sys_color = modeinfo.bits_per_pixel;
      *fbmem = mmap(0, length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
      if (*fbmem < 0) {
         perror("mmap()");
         length = 0;
      }
      return length;
}


void fb_ungrab (unsigned short **fbmem, unsigned int length)
{
      if (*fbmem) munmap(*fbmem, length);
}


void clear_display (void)
{
	int p=10;

      int *fb = (int *)fbmem;
      int idx;
      for (idx=0;idx<(XSIZE*YSIZE)/2;idx++) {	// 여기서 매번 클리어를 해줌
          *fb++ = (bg_color << 16) | bg_color;
	//	sleep(1);
	//	circle(50+p , 400-p, 50, 1, BLUE);
      	//	p=p+1;		
		//game_Stadium();	
	}
		//circle(50+p , 400, 50, 1, BLUE);
}


int draw_bit_map (int x,int y,char *file_name)
{
      int yi,xi,xe,ye,xposi,px;
      long lFileSize;
      char *pcFile,*Image_buf;
      short R,G,B;
      FILE *pFile;
      unsigned short bit_image16;
      unsigned int bit_image24,ImageSize = 0,remain = 0,temp,oldx;
      pFile = fopen(file_name, "rb");
      if (!pFile) {
         printf(" Could not open file '%s'.\n",file_name);
         return 0;
      }
      fseek(pFile, 0, SEEK_END);
      lFileSize = ftell(pFile);
      fseek(pFile, 0, SEEK_SET);
      pcFile = (char *)malloc(lFileSize);
      if (!pcFile) {
         printf(" Failed to allocate memory for the file.\n");
         return 0;
      }
      if (fread(pcFile, 1, lFileSize, pFile) != lFileSize) {
         printf(" Failed to read file '%s'.\n",file_name);
         free(pcFile);
         return 0;
      }
      fclose(pFile);
      Bit_Map = malloc(54);
      //memcpy(Bit_Map,pcFile,54);

      Bit_Map->bfType = pcFile[1] << 8 | pcFile[0];
      Bit_Map->bfSize = pcFile[5] << 24 | pcFile[4] << 16 | pcFile[3] << 8 | pcFile[2];
      Bit_Map->bfReserved1 = pcFile[7] << 8 | pcFile[6];
      Bit_Map->bfReserved2 = pcFile[9] << 8 | pcFile[8];
      Bit_Map->bfOffBits = pcFile[13] << 24 | pcFile[12] << 16 | pcFile[11] << 8 | pcFile[10];
      Bit_Map->biSize = pcFile[17] << 24 | pcFile[16] << 16 | pcFile[15] << 8 | pcFile[14];
      Bit_Map->biWidth = pcFile[21] << 24 | pcFile[20] << 16 | pcFile[19] << 8 | pcFile[18];
      Bit_Map->biHeight = pcFile[25] << 24 | pcFile[24] << 16 | pcFile[23] << 8 | pcFile[22];
      Bit_Map->biPlanes = pcFile[27] << 8 | pcFile[26];
      Bit_Map->biBitCount = pcFile[29] << 8 | pcFile[28];
      Bit_Map->biCompression = pcFile[33] << 24 | pcFile[32] << 16 | pcFile[31] << 8 | pcFile[30];
      Bit_Map->biSizeImage = pcFile[37] << 24 | pcFile[36] << 16 | pcFile[35] << 8 | pcFile[34];

      if ((Bit_Map->biWidth % 4) != 0) {
         ImageSize = Bit_Map->biWidth * Bit_Map->biHeight * 3 + Bit_Map->biHeight * (Bit_Map->biWidth % 4);
      } else {
         ImageSize = Bit_Map->biWidth * Bit_Map->biHeight * 3;
      }
      Bit_Map->biXpelsPerMeter = pcFile[41] << 24 | pcFile[40] << 16 | pcFile[39] << 8 | pcFile[38];
      Bit_Map->biYpelsPerMeter = pcFile[45] << 24 | pcFile[44] << 16 | pcFile[43] << 8 | pcFile[42];
      Bit_Map->biClrUsed = pcFile[49] << 24 | pcFile[48] << 16 | pcFile[47] << 8 | pcFile[46];
      Bit_Map->biClrImportant = pcFile[53] << 24 | pcFile[52] << 16 | pcFile[51] << 8 | pcFile[50];
      if (Bit_Map->bfType != 0x4d42) {
         free(Bit_Map);
         free(pcFile);
         printf(" This File Format is Not BMP '%s'.\n",file_name);
         return 0;
      }
      if ((Bit_Map->biCompression != 0) || (Bit_Map->biBitCount < 16)) {
         free(Bit_Map);
         free(pcFile);
         printf(" Not Support BMP. %d-%d\n",(int)Bit_Map->biCompression,(int)Bit_Map->biBitCount);
         return 0;
      }
      xe = Bit_Map->biWidth;
      ye = Bit_Map->biHeight;

      temp = Bit_Map->biSizeImage / ye;
      if (Bit_Map->biBitCount == 24) {
         remain = temp - (xe * 3);
      } else {
         if (Bit_Map->biBitCount == 16) {
            remain = temp - (xe * 2);
         }
      }
      if (remain < 0) remain = 0;
      //printf(" %s %dX%d=%d --> %d r=%d \n",file_name,xe,ye,xe*ye,Bit_Map->biSizeImage,remain);
      oldx = xe;
      if (((xe * 2) % 4) != 0) {
         xe = ((((xe * 2) / 4) + 1) * 4) / 2;
      }
      //printf(" xe = %d \n",xe);
      Image_buf = pcFile + Bit_Map->bfOffBits;
      Image_buf += Bit_Map->biSizeImage;
      //Image_buf = pcFile + Bit_Map->bfSize;
      xposi = (x + (xe - 1));
      for (yi=y;yi<(y+ye);yi++) {
          if (sys_color == 16) {
             if (remain) {
                Image_buf -= remain;
             }
          } else {
             if (Bit_Map->biBitCount == 24) {
                Image_buf -= 3;
             }
          }
          if (oldx != xe) temp = xe - 1;else temp = xe;
          for (xi=x;xi<(x+temp);xi++) {
              if (sys_color == 16) {
                 bit_image16 = 0;
                 if (Bit_Map->biBitCount == 24) {
                    Image_buf -= 3;
                    R = Image_buf[2] >> 3;  // RGB565
                    G = Image_buf[1] >> 2;
                    B = Image_buf[0] >> 3;
                    bit_image16 = (R << 11) | G << 5 | B;
                 } else {
                    Image_buf -= 2;
                    // RGB555 -> 565
                    R = (Image_buf[1] & 0x7C) << 9;
                    G = (Image_buf[1] & 0x03) << 9 | (Image_buf[0] & 0xE0) << 1;
                    B = (Image_buf[0] & 0x1F);
                    bit_image16 = R | G | B;
                    //bit_image16 = Image_buf[1] << 16 | Image_buf[0];
                 }
                 px = (xposi - (xi - x));
                 if ((px < maxx) && (yi < maxy)) SetPixel(px+disp_offset,yi,bit_image16);
              } else {
                 if (sys_color == 24) {
                    if (Bit_Map->biBitCount == 24) {
                       Image_buf -= 3;
                       bit_image24 = (Image_buf[2] << 24) | (Image_buf[1] << 16) | (Image_buf[0] << 8);
                    } else {
                       Image_buf -= 2;
                       R = (Image_buf[1] & 0xF8) >> 3;
                       G = (Image_buf[1] & 0x07) << 6 | (Image_buf[0] & 0xE0) >> 3;
                       B = (Image_buf[0] & 0x1F) << 3;
                       bit_image24 = R << 24 | G << 16 | B << 8;
                    }
                    px = (xposi - (xi - x));
                    if ((px < maxx) && (yi < maxy)) SetPixel(px+disp_offset,yi,bit_image24);
                 }
              }
          }
      }
      free(Bit_Map);
      free(pcFile);
      return 1;
}


void draw_pixel (int x, int y, unsigned short color)
{
      SetPixel(x,y,color);
}


void draw_horz_line (int x1, int x2, int y, int color)
{
      int idx;
      if ((x2 >= maxx) || (y >= maxy) || (y < 0) || (x1 < 0)) return;
      for (idx=x1;idx<(x2+1);idx++) frameBuffer[y][idx] = color;
}


void draw_vert_line (int x, int y1, int y2, int color)
{
      int idx;
      if ((x >= maxx) || (y2 >= maxy) || (x < 0) || (y1 < 0)) return;
      for (idx=y1;idx<(y2+1);idx++) frameBuffer[idx][x] = color;
}


void fill_rect (int x1, int y1, int x2, int y2, int color)
{
      int idx;
      if (x1 > x2) {
         idx = x2;
         x2 = x1;
         x1 = idx;
      }
      if (y1 > y2) {
         idx = y2;
         y2 = y1;
         y1 = idx;
      }
      for (idx=y1;idx<(y2+1);idx++) {
          draw_horz_line(x1,x2,idx,color);
      }
}


void rectangle (int x1,int y1,int x2,int y2,int color)
{
      draw_horz_line(x1,x2,y1,color);
      draw_vert_line(x2,y1,y2,color);
      draw_horz_line(x1,x2,y2,color);
      draw_vert_line(x1,y1,y2,color);
}


void drawrow (int x1, int x2, int y, int color)
{
      int temp;
      if (x1 > x2) {
         temp = x1;
         x1 = x2;
         x2 = temp;
      }
      if (x1 < 0) x1 = 0;
      if (x2 >= maxx) x2 = maxx - 1;
      while (x1 <= x2) {
            temp = MIN(clipmaxx, x2);
            draw_horz_line(x1, temp, y, color);
            x1 = temp + 1;
      }
}


void drawcol (int x,int y1,int y2,int color)
{
      int temp;
      if (y1 > y2) {
         temp = y1;
         y1 = y2;
         y2 = temp;
      }
      if (y1 < 0) y1 = 0;
      if (y2 >= maxy) y2 = maxy - 1;
      while (y1 <= y2) {
            temp = MIN(clipmaxy, y2);
            draw_vert_line(x, y1, temp, color);
            y1 = temp + 1;
      }
}


void line (int x1, int y1, int x2, int y2 ,int color)
{
      int xdelta;               
      int ydelta;             
      int xinc;                
      int yinc;                
      int rem; 
      int temp;
      if (y1 == y2) {
         if (x1 > x2) {
            temp = x1;
            x1 = x2 + 1;
            x2 = temp;
         } else --x2;
         drawrow(x1, x2, y1, color);
         return;
      }
      if (x1 == x2) {
         if (y1 > y2) {
            temp = y1;
            y1 = y2 + 1;
            y2 = temp;
         } else --y2;
         drawcol(x1, y1, y2, color);
         return;
      }
      xdelta = x2 - x1;
      ydelta = y2 - y1;
      if (xdelta < 0) xdelta = -xdelta;
      if (ydelta < 0) ydelta = -ydelta;
      xinc = (x2 > x1)? 1 : -1;
      yinc = (y2 > y1)? 1 : -1;
      draw_pixel(x1, y1, color);
      if (xdelta >= ydelta) {
         rem = xdelta / 2;
         for (;;) {
             if (x1 == x2) break;
             x1 += xinc;
             rem += ydelta;
             if (rem >= xdelta) {
                rem -= xdelta;
                y1 += yinc;
             }
             draw_pixel(x1, y1, color);
             if (x1 == x2) break;
         }
      } else {
         rem = ydelta / 2;
         for (;;) {
             if (y1 == y2) break;
             y1 += yinc;
             rem += xdelta;
             if (rem >= ydelta) {
                rem -= ydelta;
                x1 += xinc;
             }
             draw_pixel(x1, y1, color);
             if (y1 == y2) break;
         }
      }
}


void circle (int x,int y, int radius,int fill,int color)
{
      int a,b,P;
      a = 0;
      b = radius;
      P = 1 - radius;
      do {
          if (fill) {
             line(x-a,y+b,x+a,y+b,color);
             line(x-a,y-b,x+a,y-b,color);
             line(x-b,y+a,x+b,y+a,color);
             line(x-b,y-a,x+b,y-a,color);
          } else {
             draw_pixel(a+x,b+y,color);
             draw_pixel(b+x,a+y,color);
             draw_pixel(x-a,b+y,color);
             draw_pixel(x-b,a+y,color);
             draw_pixel(b+x,y-a,color);
             draw_pixel(a+x,y-b,color);
             draw_pixel(x-a,y-b,color);
             draw_pixel(x-b,y-a,color);
          }        
          if (P < 0) P += (3 + 2 * a++);else P += (5 + 2 * (a++ - b--));
      } while (a <= b);
}


void ellipse (int x0,int y0,int a0,int b0,int fill,int color)
{
      int x = 0,y = b0;
      int a = a0,b = b0;
      int a_squ = a * a;
      int two_a_squ = a_squ << 1;
      int b_squ = b * b;
      int two_b_squ = b_squ << 1;
      int d,dx,dy;
      d = b_squ - a_squ * b + (a_squ >> 2);
      dx = 0;
      dy = two_a_squ * b;
      while (dx < dy) {
            if (fill) {
               line(x0-x,y0-y,x0+x,y0-y,color);
               line(x0-x,y0+y,x0+x,y0+y,color);
            } else {
               draw_pixel(x0+x,y0+y,color);
               draw_pixel(x0-x,y0+y,color);
               draw_pixel(x0+x,y0-y,color);
               draw_pixel(x0-x,y0-y,color);
            } 
            if (d > 0) {
               y--;
               dy -= two_a_squ;
               d -= dy;
            }   
            x++;
            dx += two_b_squ;
            d += b_squ + dx;
      }
      d += (3 * (a_squ - b_squ) / 2 - (dx + dy) / 2);
      while (y >= 0) {
            if (fill) {
               line(x0-x,y0-y,x0+x,y0-y,color);
               line(x0-x,y0+y,x0+x,y0+y,color);
            } else { 
               draw_pixel(x0+x,y0+y,color);
               draw_pixel(x0-x,y0+y,color);
               draw_pixel(x0+x,y0-y,color);
               draw_pixel(x0-x,y0-y,color);
            }
            if (d < 0) {
               x++;
               dx += two_b_squ;
               d += dx;
            }
            y--;
            dy -= two_a_squ;
            d += a_squ - dy;
      }
}      


int main (int argc, char *argv[])
{
	int serv_sock, clnt_sock;
	struct sockaddr_in serv_adr, clnt_adr;
	socklen_t adr_sz;
	int str_len, i;
	char *buf;

	pthread_t t1;
	int s;

	int play_Num = 0; //접속자수
	
	struct epoll_event *ep_events;
	struct epoll_event event;
	int epfd, event_cnt;
	
	serv_sock = socket(PF_INET, SOCK_STREAM, 0);
	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family = AF_INET;
	serv_adr.sin_addr.s_addr = inet_addr("10.50.3.191");
	serv_adr.sin_port = htons(PORT);
	
	if(bind(serv_sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) == -1)
		error_handling("bind() error");
	if(listen(serv_sock, 2) == -1)
		error_handling("listen() error");

	epfd = epoll_create(EPOLL_SIZE);
	ep_events = malloc(sizeof(struct epoll_event)*EPOLL_SIZE);

	setnonblockingmode(serv_sock);
	event.events = EPOLLIN;
	event.data.fd = serv_sock;
	epoll_ctl( epfd, EPOLL_CTL_ADD, serv_sock, &event);

	s = pthread_create( &t1, NULL, threadFunc, NULL);

	while(1)
	{
		
		event_cnt = epoll_wait(epfd, ep_events, EPOLL_SIZE, -1);
		if( event_cnt == -1)
			break;

		for(i=0; i <event_cnt; ++i)
		{
			if(ep_events[i].data.fd == serv_sock)
			{
				adr_sz = sizeof(clnt_adr);
				clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_adr, &adr_sz);
				setnonblockingmode(clnt_sock);
				event.events = EPOLLIN|EPOLLET;
				event.data.fd = clnt_sock;
				epoll_ctl(epfd, EPOLL_CTL_ADD, clnt_sock, &event);
				printf("connected client : %d\n", clnt_sock);
				++play_Num;
				buf = itoa(play_Num*100, 10);
				write(clnt_sock, buf ,sizeof(buf)); 
			}
			else
			{
	//			if( play_Num == 2)
	//			{
					while(1)
					{
						str_len = read(ep_events[i].data.fd, buf, BUF_SIZE);
						buf[str_len] = '\0';
						if(str_len < 0)
						{
							if(errno == EAGAIN)
								break;
						}
						else if( str_len > 0)
						{
							printf("read data : %s\n", buf);
							strcmp_(buf);
						}
					}//while
	//			}//if( play_NUm == 2)
			}//else
		}//for
	}//while

	s = pthread_join(t1, NULL);
	close(serv_sock); close(epfd);

      return 0;
}

void game_Stadium()	{	// 게임 틀
	draw_horz_line(100, 700, 60, GREENYELLOW); 		
	draw_horz_line(100, 700, 400, GREENYELLOW); 		
	draw_vert_line(100, 60, 400, GREENYELLOW); 		
	draw_vert_line(700, 60, 400, GREENYELLOW);
	draw_vert_line(400, 60, 400, STEELBLUE);
	draw_vert_line(100, 160, 300, SEAGREEN);
	draw_vert_line(700, 160, 300, SEAGREEN);

}

void strcmp_(char *data)
{
	fill_rect( p1_x , p1_y -2 , p1_x + X_DEFFERENT , p1_y + Y_DEFFERENT+2 , BLACK);
	fill_rect( p2_x , p1_y -2 , p2_x + X_DEFFERENT , p1_y + Y_DEFFERENT+2 , BLACK);

	if( strcmp( data, "110") == 0 )//p1 up
		p1_y+=2;
	else if( strcmp( data, "120") == 0) //p1 down
		p1_y-=2;
	else if( strcmp (data, "210") == 0) //p2 up
		p2_y+=2;
	else if( strcmp(data, "220") == 0) //p2 down
		p2_y-=2;

	fill_rect(p1_x , p1_y , p1_x + X_DEFFERENT , p1_y + Y_DEFFERENT , BLUE);	
	fill_rect(p2_x , p2_y , p2_x + X_DEFFERENT , p2_y + Y_DEFFERENT , BLUE);
	//clear_display();	
}

void ball()	{

	if(  
		(move_x == 690 || move_x == 110)   ||
		( ( move_y > p1_y && move_y < p1_y + Y_DEFFERENT ) && (move_x == p1_x + X_DEFFERENT || move_x == p1_x) ) ||
		( ( move_y > p1_y && move_y < p1_y + Y_DEFFERENT ) && (move_x == p2_x + X_DEFFERENT || move_x == p2_x) )
	)	
	{
		if(upstate == 0) upstate = 1;	//왼쪽
		else		upstate = 0;	//오른쪽
	}
	if( 
		(move_y == 390 || move_y == 70) ||
		( ( move_x == p1_x +1 ) &&  (move_y ==p1_y + Y_DEFFERENT || move_y ==p1_y) ) ||
		( ( move_x == p2_x +1 ) &&  (move_y ==p1_y + Y_DEFFERENT || move_y ==p1_y) ) 
	)		
	{
		if(rightstate == 0) rightstate = 1;	//위
		else 			rightstate = 0; //아래
	}
	
	circle(move_x, move_y, 10, 1, BLACK);	//지우고

	if(upstate == 0 && rightstate == 0 )
	{				
		move_y = move_y+1;
		move_x = move_x +1;
	}
	else if(upstate == 1 && rightstate == 1 )
	{
		move_y = move_y	-1;
		move_x = move_x -1;
	}
	else if(rightstate == 0 && upstate == 1)
	{
		move_x = move_x -1;
		move_y = move_y +1;
	}
	else if(rightstate == 1 && upstate == 0 )
	{
		move_x = move_x +1;
		move_y = move_y -1;
	}
	else if((move_x == 100)&&((move_y >= 300) && (move_y <= 160) ))
	{
		move_x = 400;
		move_y = 200;
	}
	
	circle(move_x, move_y, 10, 1, BLUE);	//다시 그림
	usleep(sleep_Time);

	fill_rect(p1_x , p1_y , p1_x + X_DEFFERENT , p1_y + Y_DEFFERENT , BLUE);	//p1막대바
	fill_rect(p2_x , p1_y , p2_x + X_DEFFERENT , p1_y + Y_DEFFERENT , BLUE);	//p2막대바
		
}

void setnonblockingmode(int fd)
{
	int flag = fcntl(fd, F_GETFL, 0);
	fcntl(fd, F_SETFL, flag|O_NONBLOCK);
}

void error_handling(char *buf)
{
	fputs(buf, stderr);
	fputc('\n', stderr);
	exit(1);
}


static void *threadFunc(void *arg)
{
	int p = 1;
	int move_x=400;
	int move_y=200;
	int state = 0;
      int run_flag;
      fb_fd = open(FB_DEV_NODE, O_RDWR);
      if (fb_fd < 0) {
         printf(" FrameBuffer open fail....\n");
         goto err_exit;
      }

    if ((fb_length = fb_grab(fb_fd, &fbmem)) == 0) goto err_exit;
	printf(" Framebuffer memory address 0x%X  Size %d \r\n",(int)fbmem,fb_length);
	frameBuffer = (unsigned short (*)[XSIZE]) fbmem;
	init_data();
	run_flag = 1;
	fg_color = WHITE;
	bg_color = BLACK;
	clear_display();

	game_Stadium();

	while (run_flag) {
            //to do
		//clear_display();
		//usleep(30000);
		ball(move_x, move_y, state);
	}
	

err_exit:
      if (fb_fd) {
         fb_ungrab(&fbmem, fb_length);
         close(fb_fd);
      }
	  
	return 0;
}
