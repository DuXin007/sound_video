#include <stdio.h>
#include <pthread.h>


#include "../fun_base.h"
#include "../sound/sound_ctrol.h"
#include "../sound/play_sound.h"
#include "../video/video_ctrol.h"
#include "../video/play_video.h"
#include "../TFT_API.h"


/*tft显示示例*/
void startPage(void)
{
	//定义窗口描述符
	WIN_HANDLE DemoWindow0; 
	WIN_HANDLE DemoWindow1; 
	WIN_HANDLE DemoWindow2;
	WIN_HANDLE DemoWindow3;
	WIN_HANDLE DemoWindow4;
	WIN_HANDLE DemoWindow5;
	//屏幕初始化
	TFT_Init("/dev/fb0");
	
	DemoWindow0 = TFT_CreateWindowEx(0,0,800,480,COLOR_WHITE);
	TFT_DeleteWindow(DemoWindow0);
	//创建窗口
	DemoWindow1 = TFT_CreateWindowEx(0, 0, 800, 480, COLOR_BLACK);					
	DemoWindow2 = TFT_CreateWindowEx(100, 100, 90, 90, COLOR_BLACK);	
	DemoWindow3 = TFT_CreateWindowEx(100, 250, 90, 90, COLOR_BLACK);	
	DemoWindow4 = TFT_CreateWindowEx(250, 100, 90, 90, COLOR_BLACK);
	DemoWindow5 = TFT_CreateWindowEx(250, 250, 90, 90, COLOR_BLACK);
	
	//TFT_File_Picture(DemoWindow1, 0, 0, "./startPage/play.bmp", 0);
	
    TFT_ClearWindow(DemoWindow2);
	TFT_File_Picture(DemoWindow2, 0, 0, "./startPage/luzhi.bmp", 1);
	
	TFT_ClearWindow(DemoWindow3);
	TFT_File_Picture(DemoWindow3, 0, 0, "./startPage/luyin.bmp", 1);
	
	TFT_ClearWindow(DemoWindow4);
	TFT_File_Picture(DemoWindow4, 0, 0, "./startPage/video.bmp", 1);
	
	TFT_ClearWindow(DemoWindow5);
	TFT_File_Picture(DemoWindow5, 0, 0, "./startPage/shey.bmp", 1);

}


void startPage_button_scan(void)
{
	CUR_VAL val_single;	
	int touch_fd;

	touch_fd = open_touch_dev("/dev/event3"); 
	while(1)
	{
		//单点触摸分析程序
		analysis_event_single(touch_fd, &val_single);
		//数据有效状态标志位：0为数据无效，1为数据有效
		if(val_single.state==1)
		{
			if(getfocus(val_single, 100, 100, 90, 90))
			{
				printf("start luxiang\n");
				pthread_t videotid;
				video_button_init();
				sound_display();
				pthread_create(&videotid, NULL, video_button_scan,NULL);	
				frambuffer_cammera_demo();				
				void *videot;
				pthread_join(videotid, &videot);
			}
			else if(getfocus(val_single, 100, 250, 90, 90))
			{
				printf("start luyin\n");
				pthread_t btntid;
				sound_display();
				pthread_create(&btntid, NULL, sound_button_scan,NULL);	
				void *startP;
				pthread_join(btntid, &startP);
			}
			else if(getfocus(val_single, 250, 100, 90, 90))
			{
				printf("11111111111\n");
				video_list_init();
				pthread_t Pvidet;
				pthread_create(&Pvidet, NULL, play_video_scan,NULL);printf("11111111111\n");
				video_mp();
				void *startP;
				pthread_join(Pvidet, &startP);

			}
			else if(getfocus(val_single, 250, 250, 90, 90))
			{
				printf("play music\n");
				sound_list_init();
				pthread_t Psound1;
				pthread_create(&Psound1, NULL, play_sound_scan,NULL);
				play_wav();
				void *startP;
				pthread_join(Psound1, &startP);
			}
		}
	}
	close(touch_fd);
}
