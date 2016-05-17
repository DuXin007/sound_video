#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>

/*********************************************************************
*����:		����Ƶ�豸�ļ�
*����:		��Ƶ�豸�ļ���
*����ֵ:	��Ƶ�豸������
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
*����:		�ر���Ƶ�豸�ļ�
*����:		��Ƶ�豸�ļ�������
*����ֵ:	
			�ɹ���0
			ʧ�ܣ�-1
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
*����:		���ò�����
*����:		
			dsp_fd����Ƶ�豸�ļ�������
			sample_rate��������
*����ֵ:	
			�ɹ���0
			ʧ�ܣ�-1
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
*����:		���ò���λ��
*����:		
			dsp_fd����Ƶ�豸�ļ�������
			sample_bits������λ��
*����ֵ:	
			�ɹ���0
			ʧ�ܣ�-1
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
*����:		����������
*����:		
			dsp_fd����Ƶ�豸�ļ�������
			channels��������
*����ֵ:	
			�ɹ���0
			ʧ�ܣ�-1
*********************************************************************/
int oss_set_channels(int dsp_fd, int channels)
{
    int dsp_channel;
    int status;

    if(channels == 1    /* mono ������ */
       ||channels == 2){/* stereo ������ */
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
*����:		���û�����������(����)
*����:		
			mixer_fd������������Ƶ�豸�ļ�������
			val_left������������
			val_right������������
*����ֵ:	
			�ɹ���0
			ʧ�ܣ�-1
*ע�⣺����������������ʡ�����λ�������������������������������������������Ч 
*********************************************************************/
int oss_set_mixer_volume(int mixer_fd, int val_left, int val_right)   
{  
	int volume;
	int ret;

	/* ����������С */
	volume = (val_left << 8) | val_right;
	ret = ioctl(mixer_fd, MIXER_WRITE(SOUND_MIXER_PCM), &volume);
	if(ret < 0){
		perror("set volume error");
		return -1;
	}
    return 0;
}

/*********************************************************************
*����:		��ȡ������������(����)
*����:		
			mixer_fd������������Ƶ�豸�ļ�������
			val_left���洢�����������ı����ĵ�ַ
			val_right���洢�����������ı����ĵ�ַ
*����ֵ:	
			�ɹ���0
			ʧ�ܣ�-1
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
