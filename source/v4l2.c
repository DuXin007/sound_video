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
*功能:		帧图像处理函数（一般用来进行帧图像格式的转换）
*参数:		
			video：视频采集设备句柄
			src：视频采集设备的输出图像的首地址
			rgb：存放rgb图像数据的内存的首地址
*返回值:	无
*********************************************************************/
void video_frame_rgb(video_handler *video, const void *src, void *rgb)
{
	video_yuyv_to_rgb24(src, rgb, NULL, video->video_width, video->video_height);
}

/*********************************************************************
*功能:		帧图像处理函数（一般用来进行帧图像格式的转换）
*参数:		
			video：视频采集设备句柄
			src：视频采集设备的输出图像的首地址
			bgr：存放bgr图像数据的内存的首地址
*返回值:	无
*********************************************************************/
void video_frame_bgr(video_handler *video, const void *src, void *bgr)
{
	video_yuyv_to_rgb24(src, NULL, bgr, video->video_width, video->video_height);
}

/*********************************************************************
*功能:		帧图像处理函数（一般用来进行帧图像格式的转换）
*参数:		
			video：视频采集设备句柄
			src：视频采集设备的输出图像的首地址
			yuvuv：存放yuvuv图像数据的内存的首地址
*返回值:	无
*********************************************************************/
void video_frame_yuvuv(video_handler *video, const void *src, void *yuvuv)
{
	memcpy(yuvuv, src, video->video_width*video->video_height*2);
}

/*********************************************************************
*功能:		出错退出处理程序（退出调用进程）
*参数:		出错时打印的提示信息
*返回值:	无
*********************************************************************/
static void video_errno_exit(const char *s)
{
    fprintf(stderr, "%s error %d, %s\n", s, errno, strerror(errno));
    exit(-1);
}

/*********************************************************************
*功能:		获取一帧图像（需调用video_release_frame函数释放申请的空间）
*参数:		
			video：视频采集设备句柄
			video_frame_dispose：帧图像处理函数，用来转换帧图像的格式
			rgb：存放rgb数据的首地址
			bgr：存放bgr数据的首地址
*返回值:	无
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
		/* 从视频缓冲区的输出队列中取得一个已经保存有一帧视频数据的视频缓冲区 */
        ret = ioctl(video->video_fd, VIDIOC_DQBUF, &buf);
        if(-1 == ret){
            video_errno_exit("VIDIOC_DQBUF");
        }
        assert(buf.index < video->buffers_use_num);
        video_frame_dispose(video, video->buffers[buf.index].address, dest);
		/* 将一个空的视频缓冲区放到视频缓冲区输入队列中 */
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
*功能:		释放video_get_frame分配的空间
*参数:		
			rgb：rgb空间的首地址
			bgr：bgr空间的首地址
*返回值:	无
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
*功能:		YUV格式的像素点转换成RGB格式的像素点
*参数:		
			pixel：RGB格式的像素点的首地址
			y：亮度信号Y
			u：色差信号R-Y
			v：色差信号B-Y
			flag：rgb像素颜色的存储顺序；0: 顺序为rgb；1: 顺序为bgr
*返回值:	无
*********************************************************************/
static void video_yuv_to_rgb_pixel(unsigned char **pixel, int y, int u, int v, int flag)
{
	int rgbdata[3];
	
	#define R   0
	#define G   1
	#define B   2
	
	/*
	* YUV与RGB相互转换的公式如下（RGB取值范围均为0-255）
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
*功能:		yuyv格式的图像转rgb24格式的图像
*参数:		
			yuv：yuyv格式图像的首地址
			rgb：rgb24格式图像的首地址，（颜色顺序为RGB），若为NULL，则不转换
			bgr：rgb24格式图像的首地址，（颜色顺序为BGR），若为NULL，则不转换
			width：图像的宽度
			height：图像的高度
*返回值:	无
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
*功能:		申请视频缓冲空间(系统调用read、write方法)
*参数:		
			video：视频采集设备句柄
			buffer_size：频缓冲空间大小
*返回值:	无
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
*功能:		申请视频缓冲空间(内存映射)
*参数:		视频采集设备句柄
*返回值:	无
*********************************************************************/
static void video_init_mmap(video_handler *video)
{
    struct v4l2_requestbuffers req;
    int ret = 0;

	/* 分配视频缓冲区 */
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
	/* 获取分配视频缓冲区的地址 */
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
*功能:		申请视频缓冲空间(用户指针方式)
*参数:		
			video：视频采集设备句柄
			buffer_size：频缓冲空间大小
*返回值:	无
*********************************************************************/
static void video_init_userp(video_handler *video, unsigned int buffer_size)
{
    struct v4l2_requestbuffers req;
	int ret = 0;

	/* 分配捕获视频所需的内存 */
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
*功能:		开启视频采集设备图像捕捉功能
*参数:		视频采集设备句柄
*返回值:	无
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
		/* 将一个空的视频缓冲区放到视频缓冲区输入队列中 */
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
*功能:		关闭视频采集设备图像捕捉功能
*参数:		视频采集设备句柄
*返回值:	无
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
*功能:		打开视频采集设备文件
*参数:	
		dev_name：
			视频采集设备文件名
		block_flag：
			阻塞标志位，1为阻塞，0为非阻塞
*返回值:	视频采集设备描述符
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
    if(!S_ISCHR(st.st_mode)){/* 测试是否为设备文件 */
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
*功能:		关闭视频采集设备文件
*参数:		视频采集设备文件描述符
*返回值:	无
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
*功能:		检查视频采集设备的属性
*参数:		视频采集设备句柄
*返回值:	无
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
	/* 判断是否是视频输入设备 */
    if(!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)){
    	fprintf(stderr, "Is not a video capture device\n");
    	exit(EXIT_FAILURE);
    }
	switch(video->method){
    case IO_READ:
    	/* 判断是否支持read、write模式 */
	if(!(cap.capabilities & V4L2_CAP_READWRITE)){
		fprintf(stderr, "Do not support  read/write systemcalls\n");
		_exit(-1);
	}
    	break;
    case IO_MMAP:
    case IO_USERPTR:
	/* 判断是否支持数据流控制模式 */
	if(!(cap.capabilities & V4L2_CAP_STREAMING)){
		fprintf(stderr, "Do not support streaming I/O ioctls\n");
		_exit(-1);
	}
    	break;
    }
}

/*********************************************************************
*功能:		设置视频采集设备的图像格式
*参数:		
			video：视频采集设备句柄
			width：视频捕捉设备的宽度
			height：视频捕捉设备的高度
*返回值:	无
*********************************************************************/
static unsigned int video_set_format(video_handler *video, int width, int height)
{
	struct v4l2_format fmt;
	int ret = 0;

	VIDEO_CLEAR(fmt);
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;		/* 数据流类型 */
	fmt.fmt.pix.width = (width>>2)<<2;			/* 宽，必须是16 的倍数  */
	fmt.fmt.pix.height = (height>>2)<<2;		/* 高，必须是16 的倍数 */
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;/* 像素的格式为YUYV */
	fmt.fmt.pix.field = V4L2_FIELD_ANY;			/* 任何一个格式都支持 */
	ret = ioctl(video->video_fd, VIDIOC_S_FMT, &fmt);
	if(-1 == ret){
		video_errno_exit("VIDIOC_S_FMT");
	}
	/* 注意：VIDIOC_S_FMT 可能会改变宽度及高度 */
	video->video_width = fmt.fmt.pix.width;
	video->video_height = fmt.fmt.pix.height;
	return fmt.fmt.pix.sizeimage;
}

/*********************************************************************
*功能:		根据视频采集设备的采集方式申请视频缓冲空间
*参数:		video：视频采集设备句柄
			size：视频缓冲空间大小
*返回值:	无
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
*功能:		视频采集设备初始化
			1、打开设备文件
			2、设置视频采集设备的采集方式
			3、检查视频采集设备的属性
			4、设置视频采集设备的图像格式
			5、申请视频缓冲空间
			6、...
*参数:		
			video：视频采集设备句柄
			devname：视频采集设备名
			width：视频捕捉设备的宽度
			height：视频捕捉设备的高度
*返回值:	无
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
*功能:		视频采集设备反初始化(释放申请的空间、关设备文件)
*参数:		视频采集设备句柄
*返回值:	无
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
