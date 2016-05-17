#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>
#include <sys/stat.h>
#include <dirent.h>

#include "../fun_base.h"
#include "../TFT_API.h"
#include "../oss.h"
#include "../startPage/startPage.h"

char *sound_list[30] = {""};
int  wav_count = 0;
int wav_num = -1;
int is_play_sound = 0;
WIN_HANDLE sound_win3;


void sound_play(int n);

void sound_list_init(void)
{
	TFT_Init("/dev/fb0");
		
	WIN_HANDLE sound_win1;
	WIN_HANDLE sound_win2;

	WIN_HANDLE sound_win5;

	
	sound_win5 = TFT_CreateWindowEx(0,0,800,480,COLOR_WHITE);
	TFT_DeleteWindow(sound_win5);
	

	sound_win1 = TFT_CreateWindowEx(730, 0, 70, 50, COLOR_GREEN);	
	sound_win2 = TFT_CreateWindowEx(0, 0, 730, 50, COLOR_BLACK);
	sound_win3 = TFT_CreateWindowEx(0, 450, 800, 30, COLOR_BLACK);	
	
	TFT_ClearWindow(sound_win1);
	TFT_SetColor(sound_win1, COLOR_RED);
	TFT_SetTextPos(sound_win1, 15, 20);
	TFT_Print(sound_win1, "%s", "主界面");

	
	TFT_ClearWindow(sound_win2);
	TFT_SetColor(sound_win2, COLOR_RED);
	TFT_SetTextPos(sound_win2, 350, 20);
	TFT_Print(sound_win2, "%s", "音频列表");

	send_to_win();
}



void send_to_win(void)
{
	int x = 30;
	int i = 0;
	WIN_HANDLE slist;
	slist = TFT_CreateWindowEx(0, 50, 800, 400, COLOR_BLUE);	
	TFT_ClearWindow(slist);
	TFT_SetColor(slist, COLOR_RED);
	get_sound_List();
	for(i = 0; i < 30; i++)
	{
		if(( 0 <= i )&&( i <= 10 ))
		{
			TFT_SetTextPos(slist, 20,  x*i);
		}
		if(( i <= 20 )&&( 10 <= i ))
		{
			TFT_SetTextPos(slist, 300,  (x*(i-10)));
		}
		if(( i <= 30 )&&( 20 <= i ))
		{
			TFT_SetTextPos(slist, 600,  x*(i-20) );
		}
		TFT_Print(slist, "%s", sound_list[i]);	
	}

}

int  get_num(CUR_VAL cv)
{
	int i = 0;

	/*list 1*/
	if((0 <= cv.current_x)
		&&(50 <= cv.current_y)
		&&(cv.current_x <= 300)
		&&(cv.current_y <= 400))
		{
			i = (cv.current_y - 50)/30;
			printf("open sound path list1-----------!!%d\n", i);
			if( (0<=i)&&(i <= wav_count))
				return i;
			else 
				return -1;
		}
	if((300 <= cv.current_x)
		&&(50 <= cv.current_y)
		&&(cv.current_x <= 500)
		&&(cv.current_y <= 400))
		{
			i = (cv.current_y - 50)/30;
			i += 10;
			printf("open sound pat list 2-----------!!%d\n", i);
			if( (10<=i)&&(i <= wav_count))
				return i;
			else 
				return -1;
		}
	if((600 <= cv.current_x)
		&&(50 <= cv.current_y)
		&&(cv.current_x <= 700)
		&&(cv.current_y <= 400))
		{
			i = (cv.current_y - 50)/30;
			i += 20;
			printf("open sound path list 3-----------!!%d\n", i);
			if( (20<=i)&&(i <= wav_count))
				return i;
			else 
				return -1;
		}
	return -1;
}

void play_sound_scan(void )
{
	CUR_VAL val_single;	
	int touch_fd;
	touch_fd = open_touch_dev("/dev/event3"); 
	int i = 0;

	while(1)
	{
		
		analysis_event_single(touch_fd, &val_single);
		if(val_single.state==1)
		{
			if(getfocus(val_single,730, 0, 60, 50))
			{
				printf("To Home Page!\n");				
				startPage();
				is_play_sound = -1;
				pthread_exit(NULL);
			}
			else if( 0 <= (i = get_num(val_single)))
			{
				/* sound_play(i);
				i = 0; */
				wav_num = i;
				is_play_sound = 1;
				i = 0;
			}			
			
		}
	
	}
	close(touch_fd);
}



void get_sound_List(void)
{
	DIR *dir;
	wav_count = 0;
	struct dirent *ptr;	
	char *path = "../sound_file/";

	
	dir = opendir(path);
	while((ptr = readdir(dir)) != NULL)
	{
		//存图片名		
		if(strstr(ptr->d_name, ".wav"))	
		{			
			printf("ptr->d_name = %s\n", ptr->d_name);		
			save_sound_list(ptr->d_name);		
		}
		if(wav_count > 30)
			break;
	}
	closedir(dir);
}

void save_sound_list(char *mpg)
{
	char *path = "../sound_file/";
	sound_list[wav_count] = (char *)malloc(strlen(mpg)+1+strlen(path));
	bzero(sound_list[wav_count], strlen(mpg)+1);
	strcpy(sound_list[wav_count], path);
	strcat(sound_list[wav_count], mpg);
	wav_count++;
}


void sound_play(int n)
{
	int fd,dsp_fd,len = 0;
	char buffer[44100] = "";
	wav_header head;
	int ignore;

	fd = open(sound_list[n],O_RDWR,0777);
	dsp_fd = oss_open_device("/dev/dsp");	//打开音频设备

	lseek(fd,0,0);
	read(fd,&head,sizeof(wav_header));		//读取音频文件头信息
//	print_wav_head(head);					//打印音频文件头信息
	len = head.data_length;					//获取音频数据长度
	lseek(fd,44,0);							//跳过音频文件头部，准备播放数据
	ignore = len/sizeof(buffer);			//得到读取次数
	
	TFT_ClearWindow(sound_win3);
	TFT_SetColor(sound_win3, COLOR_RED);
	TFT_SetTextPos(sound_win3, 200, 10);
	TFT_Print(sound_win3, "%s is play.....", sound_list[n]);
	
	oss_set_sample_rate(dsp_fd, head.sample_rate);		//按照头信息配置频率等内容
	oss_set_channels(dsp_fd, head.channels);
	oss_set_sample_bits(dsp_fd, 16);
	oss_set_mixer_volume(dsp_fd, 100, 100);				//按照头信息配置音量等内容
	while(ignore)
	{
	//	if(is_sound == 0)
	//		break;
		int length = read(fd,buffer,sizeof(buffer));//从文件中读出一帧数据
		write(dsp_fd, buffer, length);			//写入音频设备，让其播放
		ignore--;
	}	
	
	TFT_ClearWindow(sound_win3);
	TFT_SetTextPos(sound_win3, 200, 10);
	TFT_Print(sound_win3, "%s is over!", sound_list[n]);
	

	close(fd);
	oss_close_device(dsp_fd);							//关闭文件及设备
}
void play_wav(void)
{
	while(1)
	{
		if(is_play_sound == 1)
			if( 0 <= wav_num )
				sound_play(wav_num);
		if(is_play_sound == -1)
		{is_play_sound = 0;  return ;}
		is_play_sound = 0;
		wav_num = -1;		usleep(10*1000);
	}
}
