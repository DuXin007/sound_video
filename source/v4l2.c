#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>

#include "v4l2.h"

#define VIDEO_CLEAR(x) memset(&(x), 0, sizeof(x))

/*********************************************************************
*����:		֡ͼ��������һ����������֡ͼ���ʽ��ת����
*����:		
			video����Ƶ�ɼ��豸���
			src����Ƶ�ɼ��豸�����ͼ����׵�ַ
			rgb�����rgbͼ�����ݵ��ڴ���׵�ַ
*����ֵ:	��
*********************************************************************/
void video_frame_rgb(video_handler *video, const void *src, void *rgb)
{
	video_yuyv_to_rgb24(src, rgb, NULL, video->video_width, video->video_height);
}

/*********************************************************************
*����:		֡ͼ��������һ����������֡ͼ���ʽ��ת����
*����:		
			video����Ƶ�ɼ��豸���
			src����Ƶ�ɼ��豸�����ͼ����׵�ַ
			bgr�����bgrͼ�����ݵ��ڴ���׵�ַ
*����ֵ:	��
*********************************************************************/
void video_frame_bgr(video_handler *video, const void *src, void *bgr)
{
	video_yuyv_to_rgb24(src, NULL, bgr, video->video_width, video->video_height);
}

/*********************************************************************
*����:		֡ͼ��������һ����������֡ͼ���ʽ��ת����
*����:		
			video����Ƶ�ɼ��豸���
			src����Ƶ�ɼ��豸�����ͼ����׵�ַ
			yuvuv�����yuvuvͼ�����ݵ��ڴ���׵�ַ
*����ֵ:	��
*********************************************************************/
void video_frame_yuvuv(video_handler *video, const void *src, void *yuvuv)
{
	memcpy(yuvuv, src, video->video_width*video->video_height*2);
}

/*********************************************************************
*����:		�����˳���������˳����ý��̣�
*����:		����ʱ��ӡ����ʾ��Ϣ
*����ֵ:	��
*********************************************************************/
static void video_errno_exit(const char *s)
{
    fprintf(stderr, "%s error %d, %s\n", s, errno, strerror(errno));
    exit(-1);
}

/*********************************************************************
*����:		��ȡһ֡ͼ�������video_release_frame�����ͷ�����Ŀռ䣩
*����:		
			video����Ƶ�ɼ��豸���
			video_frame_dispose��֡ͼ������������ת��֡ͼ��ĸ�ʽ
			rgb�����rgb���ݵ��׵�ַ
			bgr�����bgr���ݵ��׵�ַ
*����ֵ:	��
*********************************************************************/
void video_get_frame(video_handler *video, VIDEO_FRAME_CALLBACK video_frame_dispose, void *dest)
{
    struct v4l2_buffer buf;
    unsigned int i;
    int ret = 0;

    switch(video->method){
    case IO_READ:
        ret = read(video->video_fd, video->buffers->address, video->buffers->length);
        if(-1 == ret){
            video_errno_exit("read");
        }
        video_frame_dispose(video, video->buffers->address, dest);
        break;
    case IO_MMAP:
        VIDEO_CLEAR(buf);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
		/* ����Ƶ�����������������ȡ��һ���Ѿ�������һ֡��Ƶ���ݵ���Ƶ������ */
        ret = ioctl(video->video_fd, VIDIOC_DQBUF, &buf);
        if(-1 == ret){
            video_errno_exit("VIDIOC_DQBUF");
        }
        assert(buf.index < video->buffers_use_num);
        video_frame_dispose(video, video->buffers[buf.index].address, dest);
		/* ��һ���յ���Ƶ�������ŵ���Ƶ��������������� */
        ret = ioctl(video->video_fd, VIDIOC_QBUF, &buf);
        if(-1 == ret){
            video_errno_exit("VIDIOC_QBUF");
        }
        break;
    case IO_USERPTR:
        VIDEO_CLEAR(buf);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_USERPTR;
        ret = ioctl(video->video_fd, VIDIOC_DQBUF, &buf);
        if(-1 == ret){
            video_errno_exit("VIDIOC_DQBUF");
        }
        for(i = 0; i < video->buffers_use_num; ++i)
            if(buf.m.userptr == (unsigned long) video->buffers[i].address
                && buf.length == video->buffers[i].length){
            break;
        }
        assert(i < video->buffers_use_num);
        video_frame_dispose(video, (void *)buf.m.userptr, dest);
        ret = ioctl(video->video_fd, VIDIOC_QBUF, &buf);
        if(-1 == ret){
            video_errno_exit("VIDIOC_QBUF");
        }
        break;
    }
}

/*********************************************************************
*����:		�ͷ�video_get_frame����Ŀռ�
*����:		
			rgb��rgb�ռ���׵�ַ
			bgr��bgr�ռ���׵�ַ
*����ֵ:	��
*********************************************************************/
void video_release_frame(void *rgb, void *bgr)
{
	if(rgb != NULL)
	{
		free(rgb);
	}
	if(bgr != NULL)
	{
		free(bgr);
	}
}

/*********************************************************************
*����:		YUV��ʽ�����ص�ת����RGB��ʽ�����ص�
*����:		
			pixel��RGB��ʽ�����ص���׵�ַ
			y�������ź�Y
			u��ɫ���ź�R-Y
			v��ɫ���ź�B-Y
			flag��rgb������ɫ�Ĵ洢˳��0: ˳��Ϊrgb��1: ˳��Ϊbgr
*����ֵ:	��
*********************************************************************/
static void video_yuv_to_rgb_pixel(unsigned char **pixel, int y, int u, int v, int flag)
{
	int rgbdata[3];
	
	#define R   0
	#define G   1
	#define B   2
	
	/*
	* YUV��RGB�໥ת���Ĺ�ʽ���£�RGBȡֵ��Χ��Ϊ0-255��
	* Y = 0.299R + 0.587G + 0.114B
	* U = -0.147R - 0.289G + 0.436B
	* V = 0.615R - 0.515G - 0.100B
	* R = Y + 1.14V
	* G = Y - 0.39U - 0.58V
	* B = Y + 2.03U
	*/
	
	rgbdata[R] = y + (v - 128) + (((v - 128) * 104 ) >> 8);  
	rgbdata[G] = y - (((u - 128) * 89) >> 8) - (((v - 128) * 183) >> 8);  
	rgbdata[B] = y + (u - 128) + (((u - 128) * 199) >> 8);  
	
	if (rgbdata[R] > 255)  rgbdata[R] = 255;        
	if (rgbdata[R] < 0) rgbdata[R] = 0;            
	if (rgbdata[G] > 255)  rgbdata[G] = 255;        
	if (rgbdata[G] < 0) rgbdata[G] = 0;            
	if (rgbdata[B] > 255)  rgbdata[B] = 255;        
	if (rgbdata[B] < 0) rgbdata[B] = 0;
	
	if(flag == 0)
	{
		(*pixel)[0] = rgbdata[R];  
		(*pixel)[1] = rgbdata[G];  
		(*pixel)[2] = rgbdata[B];
	}else if(flag == 1)
	{
		(*pixel)[0] = rgbdata[B];
		(*pixel)[1] = rgbdata[G];
		(*pixel)[2] = rgbdata[R];
	}
	(*pixel) +=3;
}

/*********************************************************************
*����:		yuyv��ʽ��ͼ��תrgb24��ʽ��ͼ��
*����:		
			yuv��yuyv��ʽͼ����׵�ַ
			rgb��rgb24��ʽͼ����׵�ַ������ɫ˳��ΪRGB������ΪNULL����ת��
			bgr��rgb24��ʽͼ����׵�ַ������ɫ˳��ΪBGR������ΪNULL����ת��
			width��ͼ��Ŀ��
			height��ͼ��ĸ߶�
*����ֵ:	��
*********************************************************************/
void video_yuyv_to_rgb24(const void *yuyv,
						void *rgb,
						void *bgr,
						unsigned int width,
						unsigned int height)
{  
    int yuvdata[4];  
	unsigned char *yuv_buf;
    unsigned int i, j;  
	
	#define Y0  0  
	#define U   1  
	#define Y1  2  
	#define V   3  

	yuv_buf = (unsigned char *)yuyv;
	if(rgb != NULL)
	{
		for(i = 0; i < height * 2; i++){  
			for(j = 0; j < width; j+= 4){  
				/* get Y0 U Y1 V */  
				yuvdata[Y0] = *(yuv_buf + i * width + j + 0);  
				yuvdata[U]  = *(yuv_buf + i * width + j + 1);  
				yuvdata[Y1] = *(yuv_buf + i * width + j + 2);  
				yuvdata[V]  = *(yuv_buf + i * width + j + 3);  

				video_yuv_to_rgb_pixel((unsigned char **)(&rgb), yuvdata[Y0], yuvdata[U], yuvdata[V], 0);
				video_yuv_to_rgb_pixel((unsigned char **)(&rgb), yuvdata[Y1], yuvdata[U], yuvdata[V], 0);
			}
		}
	}
	if(bgr != NULL)
	{
		for(i = 0; i < height * 2; i++){  
			for(j = 0; j < width; j+= 4){  
				/* get Y0 U Y1 V */  
				yuvdata[Y0] = *(yuv_buf + i * width + j + 0);  
				yuvdata[U]  = *(yuv_buf + i * width + j + 1);  
				yuvdata[Y1] = *(yuv_buf + i * width + j + 2);  
				yuvdata[V]  = *(yuv_buf + i * width + j + 3);  

				video_yuv_to_rgb_pixel((unsigned char **)(&bgr), yuvdata[Y0], yuvdata[U], yuvdata[V], 1);
				video_yuv_to_rgb_pixel((unsigned char **)(&bgr), yuvdata[Y1], yuvdata[U], yuvdata[V], 1);
			}
		}
	} 
}

/*********************************************************************
*����:		������Ƶ����ռ�(ϵͳ����read��write����)
*����:		
			video����Ƶ�ɼ��豸���
			buffer_size��Ƶ����ռ��С
*����ֵ:	��
*********************************************************************/
static void video_init_read(video_handler *video, unsigned int buffer_size)
{
    video->buffers = calloc(1, sizeof(vedio_buffer));
    if(!video->buffers){
    	fprintf(stderr, "Out of memory\n");
    	exit(EXIT_FAILURE);
    }
    video->buffers->length = buffer_size;
    video->buffers->address = calloc(1, buffer_size);
    if(!video->buffers->address){
    	fprintf(stderr, "Out of memory\n");
    	exit(EXIT_FAILURE);
    }
}

/*********************************************************************
*����:		������Ƶ����ռ�(�ڴ�ӳ��)
*����:		��Ƶ�ɼ��豸���
*����ֵ:	��
*********************************************************************/
static void video_init_mmap(video_handler *video)
{
    struct v4l2_requestbuffers req;
    int ret = 0;

	/* ������Ƶ������ */
    VIDEO_CLEAR(req);
    req.count = video->buffers_set_num;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
	ret = ioctl(video->video_fd, VIDIOC_REQBUFS, &req);
    if(-1 == ret){
    	if(EINVAL == errno){
    	    fprintf(stderr, "Do not support memory mapping\n");
    	    exit(EXIT_FAILURE);
    	}else{
    	    video_errno_exit("VIDIOC_REQBUFS");
    	}
    }
    if(req.count < 2){
        fprintf(stderr, "Insufficient buffer memory on device\n");
        exit(EXIT_FAILURE);
    }
	/* ��ȡ������Ƶ�������ĵ�ַ */
    video->buffers = calloc(req.count, sizeof(vedio_buffer));
    if(!video->buffers){
    	fprintf(stderr, "Out of memory\n");
    	exit(EXIT_FAILURE);
    }
    for(video->buffers_use_num = 0;
		video->buffers_use_num < req.count;
		++video->buffers_use_num){
    	struct v4l2_buffer buf;

    	VIDEO_CLEAR(buf);
    	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    	buf.memory = V4L2_MEMORY_MMAP;
    	buf.index = video->buffers_use_num;
        ret = ioctl(video->video_fd, VIDIOC_QUERYBUF, &buf);
    	if(-1 == ret){
    	    video_errno_exit("VIDIOC_QUERYBUF");
    	}
    	video->buffers[video->buffers_use_num].length = buf.length;
    	video->buffers[video->buffers_use_num].address = mmap(NULL /* address anywhere */ ,
    					buf.length,
    					PROT_READ | PROT_WRITE /* required */ ,
    					MAP_SHARED /* recommended */ ,
    					video->video_fd, buf.m.offset);
    	if(MAP_FAILED == video->buffers[video->buffers_use_num].address){
    	    video_errno_exit("mmap");
    	}
    }
}

/*********************************************************************
*����:		������Ƶ����ռ�(�û�ָ�뷽ʽ)
*����:		
			video����Ƶ�ɼ��豸���
			buffer_size��Ƶ����ռ��С
*����ֵ:	��
*********************************************************************/
static void video_init_userp(video_handler *video, unsigned int buffer_size)
{
    struct v4l2_requestbuffers req;
	int ret = 0;

	/* ���䲶����Ƶ������ڴ� */
	VIDEO_CLEAR(req);
	req.count = video->buffers_set_num;;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_USERPTR;
	ret = ioctl(video->video_fd, VIDIOC_REQBUFS, &req);
	if(-1 == ret){
		if (EINVAL == errno) {
			fprintf(stderr, "Do not support "
				 "user pointer i/o\n");
			exit(EXIT_FAILURE);
		} else {
			video_errno_exit("VIDIOC_REQBUFS");
		}
	}
	video->buffers = calloc(video->buffers_set_num, sizeof(vedio_buffer));
	if (!video->buffers){
		fprintf(stderr, "Out of memory\n");
		exit(EXIT_FAILURE);
	}
	for (video->buffers_use_num = 0;
		video->buffers_use_num < video->buffers_set_num;
		++video->buffers_use_num) {
		video->buffers[video->buffers_use_num].length = buffer_size;
		video->buffers[video->buffers_use_num].address = calloc(1, buffer_size);
		if (!video->buffers[video->buffers_use_num].address) {
			fprintf(stderr, "Out of memory\n");
			exit(EXIT_FAILURE);
		}
	}
}

/*********************************************************************
*����:		������Ƶ�ɼ��豸ͼ��׽����
*����:		��Ƶ�ɼ��豸���
*����ֵ:	��
*********************************************************************/
void video_start_capturing(video_handler *video)
{
    unsigned int i;
    int ret = 0;
    enum v4l2_buf_type type;

    switch(video->method){
    case IO_READ:
    	/* Nothing to do. */
    	break;
    case IO_MMAP:
		/* ��һ���յ���Ƶ�������ŵ���Ƶ��������������� */
    	for(i = 0; i < video->buffers_use_num; ++i){
    	    struct v4l2_buffer buf;
			
    	    VIDEO_CLEAR(buf);
    	    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    	    buf.memory = V4L2_MEMORY_MMAP;
    	    buf.index = i;
            ret = ioctl(video->video_fd, VIDIOC_QBUF, &buf);
    	    if(-1 == ret){
                video_errno_exit("VIDIOC_QBUF");
    	    }
    	}
    	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        ret = ioctl(video->video_fd, VIDIOC_STREAMON, &type);
    	if(-1 == ret){
    	    video_errno_exit("VIDIOC_STREAMON");
    	}
    	break;
    case IO_USERPTR:
    	for(i = 0; i < video->buffers_use_num; ++i){
    	    struct v4l2_buffer buf;
			
    	    VIDEO_CLEAR(buf);
    	    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    	    buf.memory = V4L2_MEMORY_USERPTR;
    	    buf.index = i;
    	    buf.m.userptr = (unsigned long) video->buffers[i].address;
    	    buf.length = video->buffers[i].length;
            ret = ioctl(video->video_fd, VIDIOC_QBUF, &buf);
    	    if(-1 == ret){
                video_errno_exit("VIDIOC_QBUF");
    	    }
    	}
    	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        ret = ioctl(video->video_fd, VIDIOC_STREAMON, &type);
    	if(-1 == ret){
    	    video_errno_exit("VIDIOC_STREAMON");
    	}
    	break;
    }
}

/*********************************************************************
*����:		�ر���Ƶ�ɼ��豸ͼ��׽����
*����:		��Ƶ�ɼ��豸���
*����ֵ:	��
*********************************************************************/
void video_stop_capturing(video_handler *video)
{
    enum v4l2_buf_type type;
    int ret = 0;

    switch(video->method){
    case IO_READ:
        /* Nothing to do */
        break;
    case IO_MMAP:
    case IO_USERPTR:
        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        ret = ioctl(video->video_fd, VIDIOC_STREAMOFF, &type);
        if(-1 == ret){
            video_errno_exit("VIDIOC_STREAMOFF");
        }
        break;
    }
}

/*********************************************************************
*����:		����Ƶ�ɼ��豸�ļ�
*����:	
		dev_name��
			��Ƶ�ɼ��豸�ļ���
		block_flag��
			������־λ��1Ϊ������0Ϊ������
*����ֵ:	��Ƶ�ɼ��豸������
*********************************************************************/
static int video_open_device(char *dev_name, char block_flag)
{
    struct stat st;
	char *default_dev = "/dev/video0";
    int video_fd = 0;
    int ret = 0;
    
	if(dev_name == NULL){
		dev_name = default_dev;
	}
    ret = stat(dev_name, &st);
    if(-1 == ret){
    	video_errno_exit("get video status");
    }
    if(!S_ISCHR(st.st_mode)){/* �����Ƿ�Ϊ�豸�ļ� */
        video_errno_exit("video file st_mode");
    }
	if(block_flag == 1){
		video_fd = open(dev_name, O_RDWR);
	}else{
		video_fd = open(dev_name, O_RDWR|O_NONBLOCK);
	}
    if(-1 == video_fd){
    	video_errno_exit("open video device");;
    }
    return video_fd;
}

/*********************************************************************
*����:		�ر���Ƶ�ɼ��豸�ļ�
*����:		��Ƶ�ɼ��豸�ļ�������
*����ֵ:	��
*********************************************************************/
static void video_close_device(int video_fd)
{
    int ret = 0;

    ret = close(video_fd);
    if(-1 == ret){
        video_errno_exit("close video device");
    }
}

/*********************************************************************
*����:		�����Ƶ�ɼ��豸������
*����:		��Ƶ�ɼ��豸���
*����ֵ:	��
*********************************************************************/
static void video_check_capability(video_handler *video)
{
	struct v4l2_capability cap;
	int ret = 0;
	
	VIDEO_CLEAR(cap);
	ret = ioctl(video->video_fd, VIDIOC_QUERYCAP, &cap);
    if(-1 == ret){
    	video_errno_exit("VIDIOC_QUERYCAP");
    }
    snprintf(video->video_info, sizeof(video->video_info),
            "video driver name:\n\t%s\n"\
            "video card name:\n\t%s\n"\
            "vieo bus info:\n\t%s\n"\
            "video driver version:\n\t%u.%u.%u\n",
            cap.driver, cap.card, cap.bus_info,
            (cap.version >> 16) & 0XFF, (cap.version >> 8) & 0XFF,
            cap.version & 0XFF);
	/* �ж��Ƿ�����Ƶ�����豸 */
    if(!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)){
    	fprintf(stderr, "Is not a video capture device\n");
    	exit(EXIT_FAILURE);
    }
	switch(video->method){
    case IO_READ:
    	/* �ж��Ƿ�֧��read��writeģʽ */
	if(!(cap.capabilities & V4L2_CAP_READWRITE)){
		fprintf(stderr, "Do not support  read/write systemcalls\n");
		_exit(-1);
	}
    	break;
    case IO_MMAP:
    case IO_USERPTR:
	/* �ж��Ƿ�֧������������ģʽ */
	if(!(cap.capabilities & V4L2_CAP_STREAMING)){
		fprintf(stderr, "Do not support streaming I/O ioctls\n");
		_exit(-1);
	}
    	break;
    }
}

/*********************************************************************
*����:		������Ƶ�ɼ��豸��ͼ���ʽ
*����:		
			video����Ƶ�ɼ��豸���
			width����Ƶ��׽�豸�Ŀ��
			height����Ƶ��׽�豸�ĸ߶�
*����ֵ:	��
*********************************************************************/
static unsigned int video_set_format(video_handler *video, int width, int height)
{
	struct v4l2_format fmt;
	int ret = 0;

	VIDEO_CLEAR(fmt);
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;		/* ���������� */
	fmt.fmt.pix.width = (width>>2)<<2;			/* ��������16 �ı���  */
	fmt.fmt.pix.height = (height>>2)<<2;		/* �ߣ�������16 �ı��� */
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;/* ���صĸ�ʽΪYUYV */
	fmt.fmt.pix.field = V4L2_FIELD_ANY;			/* �κ�һ����ʽ��֧�� */
	ret = ioctl(video->video_fd, VIDIOC_S_FMT, &fmt);
	if(-1 == ret){
		video_errno_exit("VIDIOC_S_FMT");
	}
	/* ע�⣺VIDIOC_S_FMT ���ܻ�ı��ȼ��߶� */
	video->video_width = fmt.fmt.pix.width;
	video->video_height = fmt.fmt.pix.height;
	return fmt.fmt.pix.sizeimage;
}

/*********************************************************************
*����:		������Ƶ�ɼ��豸�Ĳɼ���ʽ������Ƶ����ռ�
*����:		video����Ƶ�ɼ��豸���
			size����Ƶ����ռ��С
*����ֵ:	��
*********************************************************************/
static void video_init_method(video_handler *video, unsigned int size)
{
	video->buffers_use_num = 0;
	video->buffers_set_num = 4;
	switch(video->method){
	case IO_READ:
		video_init_read(video, size);
		break;
	case IO_MMAP:
		video_init_mmap(video);
		break;
	case IO_USERPTR:
		video_init_userp(video, size);
		break;
	}
}

/*********************************************************************
*����:		��Ƶ�ɼ��豸��ʼ��
			1�����豸�ļ�
			2��������Ƶ�ɼ��豸�Ĳɼ���ʽ
			3�������Ƶ�ɼ��豸������
			4��������Ƶ�ɼ��豸��ͼ���ʽ
			5��������Ƶ����ռ�
			6��...
*����:		
			video����Ƶ�ɼ��豸���
			devname����Ƶ�ɼ��豸��
			width����Ƶ��׽�豸�Ŀ��
			height����Ƶ��׽�豸�ĸ߶�
*����ֵ:	��
*********************************************************************/
void video_init_device(video_handler *video, char *devname, int width, int height)
{
	unsigned int size = 0;

	memset(video, 0, sizeof(video_handler));
	video->video_fd = video_open_device(devname, 1);
	video->method = IO_MMAP;
	video_check_capability(video);
	size = video_set_format(video, width, height);
	video_init_method(video, size);
}

/*********************************************************************
*����:		��Ƶ�ɼ��豸����ʼ��(�ͷ�����Ŀռ䡢���豸�ļ�)
*����:		��Ƶ�ɼ��豸���
*����ֵ:	��
*********************************************************************/
void video_uninit_device(video_handler *video)
{
	unsigned int i;
	int ret = 0;

	switch(video->method){
	case IO_READ:
		free(video->buffers->address);
		break;
	case IO_MMAP:
		for(i=0; i<video->buffers_use_num; ++i){
			ret = munmap(video->buffers[i].address, video->buffers[i].length);
			if(-1 == ret){
				video_errno_exit("munmap");
			}
		}
		break;
	case IO_USERPTR:
		for(i=0; i<video->buffers_use_num; ++i){
			free(video->buffers[i].address);
		}
		break;
	}
	free(video->buffers);
	video_close_device(video->video_fd);
}
