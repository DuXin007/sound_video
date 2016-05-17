#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>

/*********************************************************************
*功能:		打开音频设备文件
*参数:		音频设备文件名
*返回值:	音频设备描述符
*********************************************************************/
int oss_open_device(char *devname)
{
	int oss_fd;

	oss_fd = open(devname, O_RDWR);
	if(oss_fd == -1){  		
		perror("open oss device");
		exit(-1); 	
	}
	return oss_fd;
}

/*********************************************************************
*功能:		关闭音频设备文件
*参数:		音频设备文件描述符
*返回值:	
			成功：0
			失败：-1
*********************************************************************/
int oss_close_device(int oss_fd)
{
	int ret;

	ret = close(oss_fd);
	if(ret == -1){  		
		perror("close oss device");
		return -1; 	
	}
    return 0;
}

/*********************************************************************
*功能:		设置采样率
*参数:		
			dsp_fd：音频设备文件描述符
			sample_rate：采样率
*返回值:	
			成功：0
			失败：-1
*********************************************************************/
int oss_set_sample_rate(int dsp_fd, long sample_rate)
{
    int status;

    printf("using %ld Hz sample rate \n", sample_rate);
	status = ioctl(dsp_fd, SNDCTL_DSP_SPEED, &sample_rate);
	if(status < 0){
		perror("ioctl SNDCTL_DSP_CHANNELS error");
        return -1;
	}
    return 0;
}

/*********************************************************************
*功能:		设置采样位数
*参数:		
			dsp_fd：音频设备文件描述符
			sample_bits：采样位数
*返回值:	
			成功：0
			失败：-1
*********************************************************************/
int oss_set_sample_bits(int dsp_fd, unsigned int sample_bits)
{
    unsigned int dsp_bits;
    int status;

    switch(sample_bits){
    case 8:
        dsp_bits = AFMT_U8;
        break;
    case 16:
        dsp_bits = AFMT_S16_LE;
        break;
    case 24:
        //undefined AFMT_S16_LE insteaded
        dsp_bits = AFMT_S16_LE;
        break;
    default:
        dsp_bits = AFMT_S16_LE;
        break;
    }
    printf("sample bits = %d\n", dsp_bits);
	status = ioctl(dsp_fd, SNDCTL_DSP_SETFMT, &dsp_bits);
	if(status < 0){
		perror("ioctl SNDCTL_DSP_SETFMT error");
        return -1;
	}
    return 0;
}

/*********************************************************************
*功能:		设置声道数
*参数:		
			dsp_fd：音频设备文件描述符
			channels：声道数
*返回值:	
			成功：0
			失败：-1
*********************************************************************/
int oss_set_channels(int dsp_fd, int channels)
{
    int dsp_channel;
    int status;

    if(channels == 1    /* mono 单声道 */
       ||channels == 2){/* stereo 立体声 */
        dsp_channel = channels;
    }else{
        dsp_channel = 2;
    }
    printf("channels = %d\n", dsp_channel);
	status = ioctl(dsp_fd, SNDCTL_DSP_CHANNELS, &dsp_channel);
	if(status < 0){
		perror("ioctl SNDCTL_DSP_CHANNELS error");
        return -1;
	}
    return 0;
}

/*********************************************************************
*功能:		设置混音器的音量(放音)
*参数:		
			mixer_fd：混音器或音频设备文件描述符
			val_left：左声道声音
			val_right：右声道声音
*返回值:	
			成功：0
			失败：-1
*注意：必需在设置完采样率、量化位数、声道数后才能设置声音，否则设置声音音量无效 
*********************************************************************/
int oss_set_mixer_volume(int mixer_fd, int val_left, int val_right)   
{  
	int volume;
	int ret;

	/* 设置音量大小 */
	volume = (val_left << 8) | val_right;
	ret = ioctl(mixer_fd, MIXER_WRITE(SOUND_MIXER_PCM), &volume);
	if(ret < 0){
		perror("set volume error");
		return -1;
	}
    return 0;
}

/*********************************************************************
*功能:		获取混音器的音量(放音)
*参数:		
			mixer_fd：混音器或音频设备文件描述符
			val_left：存储左声道声音的变量的地址
			val_right：存储右声道声音的变量的地址
*返回值:	
			成功：0
			失败：-1
*********************************************************************/
int oss_get_mixer_volume(int mixer_fd, int *val_left, int *val_right) 
{
    int volume;
	int ret;

	ret = ioctl(mixer_fd, MIXER_READ(SOUND_MIXER_PCM), &volume); 
	if(ret < 0){
		perror("get volume error");
		return -1;
	}else{
        *val_left = ((volume&0xff00)>>8);
        *val_right = (volume&0x00ff);
		printf("volume: left = %d, right = %d\n", *val_left, *val_right);
	}
    return 0;
}
