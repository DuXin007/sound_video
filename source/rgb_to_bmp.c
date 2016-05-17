#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>

#include "rgb_to_bmp.h"

typedef struct{
    unsigned short flag;		/* 低8位为字母’B’，高8位为字母’M */
    unsigned int file_size;		/* 文件大小 */
    unsigned short reversed_1;	/* 保留字1，必须设置为0 */
    unsigned short reversed_2;	/* 保留字2，必须设置为0 */
    unsigned long data_offset;	/* 图象数据在文件的位置，单位：字节（偏移量） */
}__attribute__((packed)) bmp_file;

/*
* __attribute__((packed))是GCC特有的语法，
* 用来告诉编译器，取消在编译过程中的优化对齐，
* 按照实际占用字节数进行存储。
*/

typedef struct{
    /* 当前结构体的大小，通常是40或56字节 */
	unsigned int info_size;
	/* 位图宽度，单位：像素 */
    unsigned int bmp_width;
	/* 位图高度，单位：像素 */
    unsigned int bmp_height;
	/* 色彩平面数，只有 1 为有效值 */
    unsigned short planes;
	/* 色深，即每个像素所占位数(bits per pixel)典型值为：1、4、8、16、24、32 */
    unsigned short bpp;
	/* 所使用的压缩方法，一般其值为0，即不压缩 */
    unsigned int compression;
	/* 图像大小。指原始位图数据的大小，千万别以为是文件大小 */
    unsigned int bmp_size;
	/* 图像的横向分辨率，单位为像素每米（pixels-per-meter） */
    unsigned int h_ppm;
	/* 图像的纵向分辨率，单位为像素每米 */
    unsigned int v_ppm;
	/* 调色板的颜色数，其值为0时表示颜色数为2^色深个 */
    unsigned int colors;
	/* 重要颜色数，其值等于颜色数时（或者等于0时），表示所有颜色都一样重要；通常设置为0 */
    unsigned int i_colors;
}__attribute__((packed)) bmp_info;

/*********************************************************************
*功能:		获得BMP图片的文件头及信息头
*参数:		
			width：BMP图片的宽度
			height：BMP图片的高度
*返回值:	存储BMP图片数据的内存的首地址
*********************************************************************/
static unsigned char *get_bmp_file_and_info(unsigned int width, unsigned int height)
{
    bmp_file file;
    bmp_info info;
	unsigned char *tmp = NULL;
	
	tmp = (unsigned char *)calloc(1, width*height*3+sizeof(file) + sizeof(info));
    bzero(&file, sizeof(file));
    bzero(&info, sizeof(info));
    file.flag = 0x4d42;	/* 即"BM" */
    file.file_size = sizeof(file) + sizeof(info) + width*height*3;
    file.data_offset = sizeof(file) + sizeof(info);
    info.info_size = sizeof(info);
    info.bmp_width = width;
    info.bmp_height = height;
    info.planes = 1;
    info.bpp = 24;
    info.compression = 0;
    info.bmp_size = width*height*3;
    info.h_ppm = 0;
    info.v_ppm = 0;
    info.colors = 0;
    info.i_colors = 0;
    assert(tmp);
    memcpy(tmp, &file, sizeof(file));
    memcpy(tmp+sizeof(file), &info, sizeof(info));
	return tmp;
}

/*********************************************************************
*功能:		获得BMP图片数据
*参数:		
			dest：存储BMP图片数据的内存的首地址
			rgb24：BMP图片原始位图数据
			width：BMP图片的宽度
			height：BMP图片的高度
*返回值:	存储BMP图片数据的内存的首地址
*********************************************************************/
static unsigned char *get_bmp_data(unsigned char *dest,
							unsigned char *rgb24,
							unsigned int width,
							unsigned int height)
{
    int i,j,k = 0;
	unsigned char *rgb_tmp = NULL;
	unsigned char *tmp = NULL;
	
	#define R   0
	#define G   1
	#define B   2
	
	tmp = dest + sizeof(bmp_file) + sizeof(bmp_info);
	rgb_tmp = rgb24;
    for(j=height;j>0;j--){
		rgb_tmp = (j-1)*width*3+rgb24;
        for(i=0;i<width;i++){
			/* bmp文件中的数据存放方法：从下至上，从左至右，BGR的顺序 */
			memcpy(tmp+k, rgb_tmp, 3);
			(tmp+k)[0] = rgb_tmp[B];
			(tmp+k)[1] = rgb_tmp[G];
			(tmp+k)[2] = rgb_tmp[R];
            k += 3;
			rgb_tmp +=3;
        }
    }
	return dest;
}

/*********************************************************************
*功能:		打开BMP图片文件，若文件不存在则创建，若存在则清空其内容
*参数:		BMP图片文件的文件名
*返回值:	BMP图片文件的文件描述符
*********************************************************************/
static int open_bmp_file(char *bmp_name)
{
    int bmp_fd;
	
	bmp_fd = open(bmp_name, O_CREAT|O_RDWR|O_TRUNC, 0777);
    if(bmp_fd < 0){
		perror("open");
		return -1;
    }
	return bmp_fd;
}

/*********************************************************************
*功能:		向BMP图片文件中写入图片的数据
*参数:		
			bmp_fd：BMP图片文件的文件描述符
			bmp_data：BMP图片的数据
			width：BMP图片的宽度
			height：BMP图片的高度
*返回值:	无
*********************************************************************/
static void write_bmp_file(int bmp_fd, unsigned char *bmp_data, int width, int height)
{	
	int ret;
	
	ret = write(bmp_fd, bmp_data, height*width*3 + sizeof(bmp_file) + sizeof(bmp_info));
	if(ret < 0){
		perror("write");
	}
    close(bmp_fd);
}

/*********************************************************************
*功能:		指定宽度及高度的24位RGB图像保存为BMP图片
*参数:		
			rgb24：24位RGB图像
			bmp_name：BMP图片的名字
			width：RGB图像的宽度
			height：RGB图像的高度
*返回值:	无
*********************************************************************/
void rgb24_to_bmp(unsigned char *rgb24, char *bmp_name, int width, int height)
{
    int bmp_fd;
	unsigned char *bmp = NULL;
    
	if(bmp_name == NULL){
		bmp_name = "tmp.bmp";
	}
    printf("bmp_name = %s\n", bmp_name);
    bmp_fd = open_bmp_file(bmp_name);
	bmp = get_bmp_file_and_info(width, height);
    get_bmp_data(bmp, rgb24, width, height);
	write_bmp_file(bmp_fd, bmp, width, height);
	free(bmp);
}

/*********************************************************************
*功能:		获得时间标签
*参数:		
			buf：存储时间标签的内存的首地址
			length：时间标签的内存区域的长度
*返回值:	存储时间标签的内存的首地址
*********************************************************************/
char *create_time_tag(char *buf, int length)
{
    time_t now;
	struct tm *tnow = NULL;

    now = time(NULL);
    tnow = localtime(&now);
    memset(buf, 0, length);
    snprintf(buf, length,"%02d%02d%02d%02d%02d%02d.bmp",
             1900+tnow->tm_year,
             tnow->tm_mon+1,
             tnow->tm_mday,
             tnow->tm_hour,
             tnow->tm_min,
             tnow->tm_sec);
    return buf;
}
