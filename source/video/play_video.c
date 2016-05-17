#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <pthread.h>
#include <sys/types.h>  
#include <sys/stat.h>  
#include <string.h>
#include <fcntl.h>
#include <dirent.h>

#include "../TFT_API.h"
#include "../v4l2.h"
#include "../rgb_to_bmp.h"
#include "../yuyv_rgb_jpeg.h"
#include "../startPage/startPage.h"

char *video_list[10] = {""};
int count = 0;
int mp_num = -1;
int is_mp_play = 0;

void video_list_init(void)
{
	TFT_Init("/dev/fb0");
		
	WIN_HANDLE video_win1;
	WIN_HANDLE video_win2;
	WIN_HANDLE video_win5;

	
	video_win5 = TFT_CreateWindowEx(0,0,800,480,COLOR_WHITE);
	TFT_DeleteWindow(video_win5);
	

	video_win1 = TFT_CreateWindowEx(110, 0, 50, 50, COLOR_GREEN);	
	video_win2 = TFT_CreateWindowEx(0, 0, 100, 50, COLOR_BLACK);	
	
	TFT_ClearWindow(video_win1);

	TFT_File_Picture(video_win1, 0, 0, "./home.bmp", 1);

	
	TFT_ClearWindow(video_win2);
	TFT_SetColor(video_win2, COLOR_RED);
	TFT_SetTextPos(video_win2, 0, 0);
	TFT_Print(video_win2, "%s", "\n    视频列表");

	print_to_win();
}



void print_to_win(void)
{
	int x = 38;
	int i = 0;
	WIN_HANDLE vlist;
	TFT_ClearWindow(vlist);
	vlist = TFT_CreateWindowEx(0, 50, 160, 430, COLOR_BLUE);
	TFT_SetColor(vlist, COLOR_RED);
	get_video_List();
	for(i = 0; i < 10; i++)
	{
			//vlist[i] = TFT_CreateWindowEx(0, (100 + x*i), 160, 38, COLOR_BLUE);	
		TFT_SetTextPos(vlist, 0,  x*i);		
		TFT_Print(vlist, "%s\n", video_list[i]);	
	}

}

int  get_list_num(CUR_VAL cv)
{
	int i = 0;
	if((cv.current_x <= 160)&&(cv.current_y>=50))
	{
		i = (cv.current_y - 50)/38;
		printf("open video path fi-----------!!%d\n", i);
		if( (0<=i)&&(i <= count))
			return i;
	}
	return -1;
}

void *play_video_scan(void )
{
	CUR_VAL val_single;	
	int touch_fd;
	touch_fd = open_touch_dev("/dev/event3"); 
	int i = 0;
	int n = 0;
	
	while(1)
	{
		
		analysis_event_single(touch_fd, &val_single);
		if(val_single.state==1)
		{
			if(getfocus(val_single,100, 0, 60, 50))
			{
				printf("To Home Page!\n");				
				startPage();
				is_mp_play = -1;
				//startPage_button_scan();
				for(;n<=count;n++)
					free(video_list[n]);
				pthread_exit(NULL);
			}
			else if( 0 <=(i = get_list_num(val_single)))
			{
				printf("this function is ok !!%d\n", i);
				/* play_video(i); */
				mp_num = i;
				is_mp_play = 1;
				i = 0;
			}			
		
		}
	}
	close(touch_fd);
	
}



void get_video_List(void)
{
	DIR *dir;
	count = 0;
	struct dirent *ptr;	
	char *path = "../video_file/";

	
	dir = opendir(path);
	while((ptr = readdir(dir)) != NULL)
	{
		//存图片名		
		if(strstr(ptr->d_name, ".mp4"))	
		{			
			printf("ptr->d_name = %s\n", ptr->d_name);		
			save_video_list(ptr->d_name);		
		}
	}
	closedir(dir);
}

void save_video_list(char *mpg)
{
	char *path = "../video_file/";
	video_list[count] = (char *)malloc(strlen(mpg)+1+strlen(path));
	bzero(video_list[count], strlen(mpg)+1);
	strcpy(video_list[count], path);
	strcat(video_list[count], mpg);
	count++;
}


void play_video(int n)
{
	int fd = 0;
	int len = 0;
	int i,j;
	unsigned char * rgb = NULL;
	unsigned char * rgb_map = NULL;
	unsigned char* jpgbuf = NULL;
	unsigned char* jpgbuf_map = NULL;
	
	#define B   0
	#define G   1
	#define R   2
	
	TFT_Init("/dev/fb0");
	WIN_HANDLE DemoWindow;
	DemoWindow = TFT_CreateWindowEx(160, 0, 640 , 480, COLOR_GREEN);
	TFT_ClearWindow(DemoWindow);
	

	fd = open(video_list[n], O_RDWR|O_APPEND);
	if( fd<0 )
	{
		printf("open video path filed!!\n");
		return ;
	}
	
	jpgbuf = (unsigned char *)calloc(1, 640 * 480 * 4);
	rgb = (unsigned char *)calloc(1, 640 * 480 * 3);
	while(1)
	{
		jpgbuf_map = jpgbuf;
		rgb_map = rgb;

		if(read(fd, &len,sizeof(int)) < 0)
		{
			printf("open video end!!\n");
			break ;
		}
		if( len <= -1 )
			break;
		printf("len's len is    %d\n",len);
		read(fd, jpgbuf_map, len);
		
		i = 640;
		j = 480;
	
		i = decompress_jpeg_buffer_to_rgb(jpgbuf, len, &rgb_map, &i, &j);
		
		jpgbuf_map = jpgbuf;
		for(i = 0;i < 640; i++)
		{
			for(j = 0;j < 480; j++)
			{
				jpgbuf_map[0] = rgb_map[R];
				jpgbuf_map[1] = rgb_map[G];
				jpgbuf_map[2] = rgb_map[B];
				jpgbuf_map += 4;
				rgb_map += 3;
			}
		}
		
		TFT_PutImageEx(DemoWindow, 0, 0, 640, 480, jpgbuf);		
	}
	printf("this video is play over!!\n");
	video_release_frame(rgb, NULL);
	close(fd);
	free(jpgbuf);	
}
void video_mp(void)
{	
	while(1)
	{
		if(is_mp_play == 1)
			if( 0 <= mp_num )
			{play_video(mp_num);}
		if(is_mp_play == -1)
		{is_mp_play = 0;return ;}
		mp_num = -1;
		is_mp_play = 0;usleep(10*1000);
	}
}
/* 
int main()
{
	play_video();
	return 0;
} */