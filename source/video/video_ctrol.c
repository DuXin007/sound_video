#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <pthread.h>
#include <sys/types.h>  
#include <sys/stat.h>  
#include <string.h>
#include <fcntl.h>

#include "../fun_base.h"
#include "../TFT_API.h"
#include "../v4l2.h"
#include "../rgb_to_bmp.h"
#include "../yuyv_rgb_jpeg.h"
#include "../startPage/startPage.h"

video_handler video;
int is_video = 0;
int is_video_on = 0;


	

void video_button_init(void)
{
	TFT_Init("/dev/fb0");
		
	WIN_HANDLE video_win1;
	WIN_HANDLE video_win2;
	WIN_HANDLE video_win3;
	WIN_HANDLE video_win4;
	WIN_HANDLE video_win5;
	WIN_HANDLE video_win0;
	
	video_win5 = TFT_CreateWindowEx(0,0,800,480,COLOR_WHITE);
	TFT_DeleteWindow(video_win5);
	
	video_win0 = TFT_CreateWindowEx(0, 0, 160, 480, COLOR_RED);
	video_win1 = TFT_CreateWindowEx(0, 0, 100, 50, COLOR_GREEN);	
	video_win2 = TFT_CreateWindowEx(0, 100, 100, 50, COLOR_GREEN);	
	video_win3 = TFT_CreateWindowEx(0, 200, 100, 50, COLOR_GREEN);
	video_win4 = TFT_CreateWindowEx(0, 300, 100, 50, COLOR_GREEN);
	
	TFT_ClearWindow(video_win1);
	TFT_SetColor(video_win1, COLOR_RED);
	TFT_SetTextPos(video_win1, 0, 0);
	TFT_Print(video_win1, "%s", "\n     开始");
	//TFT_File_Picture(video_win1, 0, 0, "./sound/start.bmp", 1);
	
	TFT_ClearWindow(video_win2);
	TFT_SetColor(video_win2, COLOR_RED);
	TFT_SetTextPos(video_win2, 0, 0);
	TFT_Print(video_win2, "%s", "\n     停止");
	//TFT_File_Picture(video_win2, 0, 0, "./sound/stop.bmp", 1);


	TFT_ClearWindow(video_win3);
	TFT_SetColor(video_win3, COLOR_RED);
	TFT_SetTextPos(video_win3, 0, 0);
	TFT_Print(video_win3, "%s", "\n    保存");
	//TFT_File_Picture(video_win3, 0, 0, "./sound/save.bmp", 1);
	
	TFT_ClearWindow(video_win4);
	TFT_SetColor(video_win4, COLOR_RED);
	TFT_SetTextPos(video_win4, 0, 0);
	TFT_Print(video_win3, "%s", "\n    主界面");
	//TFT_File_Picture(video_win4, 0, 0, "./sound/home.bmp", 1);

}
char *date_to_name(char *buf, int length)
{
    time_t now;
	struct tm *tnow = NULL;
    now = time(NULL);
    tnow = localtime(&now);
    memset(buf, 0, length);
    snprintf(buf, length,"%s%02d%02d%02d%02d%02d%02d%s",
			"../video_file/",
             1900+tnow->tm_year,
             tnow->tm_mon+1,
             tnow->tm_mday,
             tnow->tm_hour,
             tnow->tm_min,
             tnow->tm_sec,
			 ".mp4");
    return buf;
}

void save_video(void)
{
	char *name = NULL;
	name = (char *)malloc(sizeof(char)*40);
	name = date_to_name(name,40);
	if(rename("./test.mp4", name) == 0)
	{
		printf("save file success\n");
	}
	free(name);
	
}

void video_add_end(void)
{
	int fd = 0;
	int end = -1;
	if( (fd = open("./test.mp4", O_RDWR))<0 )
	{
		printf("open test.mp4 filed!!\n");
		return ;
	}
	lseek(fd, 0, SEEK_END);
	write(fd, &end, sizeof(int));
	return;
}

void video_button_scan(void)
{
	CUR_VAL val_single;	
	int touch_fd;
	
	touch_fd = open_touch_dev("/dev/event3"); 
	while(1)
	{
		analysis_event_single(touch_fd, &val_single);
		if(val_single.state==1)
		{
			if(getfocus(val_single,0, 0, 100, 50))
			{
				printf("begain luzhi\n");
				is_video = 1;
				
			}
			else if(getfocus(val_single, 0, 100, 100, 50))
			{
				printf("stop luzhi\n");
				is_video = 0;
			}
			else if(getfocus(val_single, 0, 200, 100, 50))
			{
				printf("to zhujiemian\n");
				video_add_end();
				save_video();
				
			}
			else if(getfocus(val_single, 0, 300, 100, 50))
			{
				printf("to zhujiemian\n");
				is_video_on = 0;
				is_video = 0;
				startPage();
				//startPage_button_scan();
				pthread_exit(NULL);
			}
		}
	}
	close(touch_fd);
}

void cammera_to_screen(video_handler *video, int x, int y)
{
	unsigned char * rgb = NULL;
	unsigned char * rgb_map = NULL;
	unsigned char * pimage = NULL;
	unsigned char * pimage_map = NULL;
	int len = 0;
	int fd = 0;
	//TFT_Init("/dev/fb0");
	WIN_HANDLE DemoWindow;
	DemoWindow = TFT_CreateWindowEx(x, y, video->video_width, video->video_height, COLOR_GREEN);
	TFT_ClearWindow(DemoWindow);
	int i, j;
	#define B   0
	#define G   1
	#define R   2
	
	fd = open("./test.mp4", O_RDWR|O_CREAT|O_APPEND, S_IWUSR);
	if(fd < 0)
	{
		printf("open path filed!\n");
		return ;
	}
	
	rgb = (unsigned char *)calloc(1, video->video_width * video->video_height * 3);
	pimage = (unsigned char *)calloc(1, video->video_width * video->video_height * 4);
	printf("statr  while\n");
	while(1)
	{
		video_get_frame(video, video_frame_rgb, rgb);
		rgb_map = rgb;
		pimage_map = pimage;
		for(i = 0;i < video->video_height; i++)
		{
			for(j = 0;j < video->video_width; j++)
			{
				pimage_map[0] = rgb_map[R];
				pimage_map[1] = rgb_map[G];
				pimage_map[2] = rgb_map[B];
				pimage_map += 4;
				rgb_map += 3;
			}
		}
		TFT_PutImageEx(DemoWindow, 0, 0, video->video_width, video->video_height, pimage);
		if(is_video == 1)
		{
			if(access("./test.mp4", W_OK) < 0)
			{
				fd = open("./test.mp4", O_RDWR|O_CREAT|O_APPEND, S_IWUSR);
			}
			rgb_map = rgb;
			pimage_map = pimage;
			len = compress_rgb24_to_jpeg_buffer(rgb_map, video->video_width, video->video_height, pimage, video->video_width * video->video_height * 3, 100);
			printf("len's len is    %d\n",len);
			if(len <= 0)
			{
				printf("video finish\n");
				break;
			}
				write(fd, &len, sizeof(int)); 
				write(fd, pimage, len);
				
			printf("is writting !!\n");
		}
		if(is_video_on == 0)
			break;
	}
	video_release_frame(rgb, NULL);
	close(fd);
	free(pimage);
}

void frambuffer_cammera_demo(void)
{
	is_video_on = 1;
	video_init_device(&video, "/dev/video0", 640, 480);
	video_start_capturing(&video);//video_save_file(&video);
	cammera_to_screen(&video, 160, 0);
	video_stop_capturing(&video);	
	video_uninit_device(&video);
}

/* int main()
{
	pthread_t btntid;
	video_button_init();
	printf("init successful!\n");
	pthread_create(&btntid, NULL, video_button_scan,NULL);
	frambuffer_cammera_demo();
	return 0;
}
 */