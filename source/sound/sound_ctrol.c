#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>
#include <sys/stat.h>

#include "../fun_base.h"
#include "../TFT_API.h"
#include "../oss.h"
#include "../startPage/startPage.h"

int is_sound = 0;

char *create_file_time_tag(char *buf, int length)
{
    time_t now;
	struct tm *tnow = NULL;
    now = time(NULL);
    tnow = localtime(&now);
    memset(buf, 0, length);
    snprintf(buf, length,"%s%02d%02d%02d%02d%02d%02d%s",
			"../sound_file/",
             1900+tnow->tm_year,
             tnow->tm_mon+1,
             tnow->tm_mday,
             tnow->tm_hour,
             tnow->tm_min,
             tnow->tm_sec,
			 ".wav");
    return buf;
}

void sound_add_head(void)
{
	struct stat statbuf;
	stat("./test.wav", &statbuf);
	printf("statbuf.st_size iss --------------\n", statbuf.st_size);
	if( statbuf.st_size > 4 )
	{
		return ;
	}
	wav_header head;
	int fd;
	
	fd = open("./test.wav",O_RDWR|O_CREAT,0777);//创建文件用于保存wav音频数据，也就是本程序的最终目的
	strcpy((char *)head.riff,"RIFF");			//填写wav文件头的相关信息
	head.file_length = 36;
	strcpy((char *)head.wav_flag,"WAVE");
	strcpy((char *)head.fmt_flag,"fmt ");

	head.transition = 0x10;
	head.format_tag = 0x01;
	head.channels = 2;
	head.sample_rate = 22050;
	head.wave_rate = 88200;
	head.block_align = 4;
	head.sample_bits = 16;
	strcpy((char *)head.data_flag,"data");
	head.data_length = 0;
	//lseek(fd,0,0);
	write(fd, &head, sizeof(head));				//主要配置信息填写完成，将其写入wav文件中
	printf("Add head success!!\n");
	close(fd);
}
void sound_recond()
{
	sound_add_head();
	int fd;
	int dsp_fd;
	int length = 0;
	int n = 0;
	char buffer[44100] = "";
	wav_header head;

	dsp_fd = oss_open_device("/dev/dsp");	
	fd = open("./test.wav",O_RDWR|O_CREAT,0777);

	oss_set_sample_rate(dsp_fd, 22050);			//设置音频设备的采样信息
	oss_set_sample_bits(dsp_fd, 16);
	oss_set_channels(dsp_fd, 2);
	oss_set_mixer_volume(dsp_fd, 100, 100);
		
	lseek(fd, 0, SEEK_END);
	
	while(1)
	{	
		if(is_sound == 0)
		{
			printf("recond sound end!!\n");
			break;
		}	
		length = read(dsp_fd, buffer, sizeof(buffer));	//从设备中读取一帧音频数据
		write(fd, buffer, length);						//将音频数据写入wav文件中
		n++;
		printf("recond sound n is ----%d\n",n);		
	}
	
	lseek(fd,0,0);
	read(fd,&head,sizeof(wav_header));	
	long ll = n*length+44;						//记录数据总长度	
	head.file_length = ll-8;
	head.data_length = ll-44;
	lseek(fd,0,0);
	write(fd,&head,sizeof(wav_header));			//将数据长度写入头信息中
	
	close(fd);
	oss_close_device(dsp_fd);
}

void save_sound(void)
{
	char *name = NULL;
	name = (char *)malloc(sizeof(char)*40);
	name = create_file_time_tag(name,40);
	if(rename("./test.wav", name) == 0)
	{
		printf("save file success\n");
	}
	free(name);
}
	



void sound_display(void)
{
	TFT_Init("/dev/fb0");
	
	WIN_HANDLE sound_win1;
	WIN_HANDLE sound_win2;
	WIN_HANDLE sound_win3;
	WIN_HANDLE sound_win4;
	WIN_HANDLE sound_win5;
	WIN_HANDLE sound_win6;
	WIN_HANDLE sound_win0;
	
	sound_win5 = TFT_CreateWindowEx(0,0,800,480,COLOR_WHITE);
	TFT_DeleteWindow(sound_win5);
	
	sound_win0 = TFT_CreateWindowEx(0, 0, 160, 480, COLOR_RED);
	sound_win1 = TFT_CreateWindowEx(0, 0, 100, 50, COLOR_GREEN);	
	sound_win2 = TFT_CreateWindowEx(0, 100, 100, 50, COLOR_GREEN);	
	sound_win3 = TFT_CreateWindowEx(0, 200, 100, 50, COLOR_GREEN);
	sound_win4 = TFT_CreateWindowEx(0, 300, 100, 50, COLOR_GREEN);
	sound_win6= TFT_CreateWindowEx(160, 0, 640, 480, COLOR_GREEN);
	
	
	TFT_File_Picture(sound_win6, 0, 0, "./sound/sound_p.bmp", 0);
	
	TFT_ClearWindow(sound_win1);
	TFT_SetColor(sound_win1, COLOR_RED);
	TFT_SetTextPos(sound_win1, 0, 0);
	TFT_Print(sound_win1, "%s", "\n     开始");
	
	TFT_ClearWindow(sound_win2);
	TFT_SetColor(sound_win2, COLOR_RED);
	TFT_SetTextPos(sound_win2, 0, 0);
	TFT_Print(sound_win2, "%s", "\n     停止");


	TFT_ClearWindow(sound_win3);
	TFT_SetColor(sound_win3, COLOR_RED);
	TFT_SetTextPos(sound_win3, 0, 0);
	TFT_Print(sound_win3, "%s", "\n    保存");
	
	TFT_ClearWindow(sound_win4);
	TFT_SetColor(sound_win4, COLOR_RED);
	TFT_SetTextPos(sound_win4, 0, 0);
	TFT_Print(sound_win4, "%s", "\n    主界面");
}

void sound_button_scan(void)
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
				is_sound = 1;
				pthread_t startid;
				pthread_create(&startid, NULL, sound_recond,NULL);
			}
			else if(getfocus(val_single, 0, 100, 100, 50))
			{
				printf("stop luzhi\n");
				is_sound = 0;
			}
			else if(getfocus(val_single, 0, 200, 100, 50))
			{
				printf("save to file\n");
				save_sound();
			}
			else if(getfocus(val_single, 0, 300, 100, 50))
			{
				printf("to HomePage\n");
				startPage();
				is_sound = 0;
				//startPage_button_scan();
				pthread_exit(NULL);
				
			}
		}
	}
	close(touch_fd);
}

