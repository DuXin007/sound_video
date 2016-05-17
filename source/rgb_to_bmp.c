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
    unsigned short flag;		/* ��8λΪ��ĸ��B������8λΪ��ĸ��M */
    unsigned int file_size;		/* �ļ���С */
    unsigned short reversed_1;	/* ������1����������Ϊ0 */
    unsigned short reversed_2;	/* ������2����������Ϊ0 */
    unsigned long data_offset;	/* ͼ���������ļ���λ�ã���λ���ֽڣ�ƫ������ */
}__attribute__((packed)) bmp_file;

/*
* __attribute__((packed))��GCC���е��﷨��
* �������߱�������ȡ���ڱ�������е��Ż����룬
* ����ʵ��ռ���ֽ������д洢��
*/

typedef struct{
    /* ��ǰ�ṹ��Ĵ�С��ͨ����40��56�ֽ� */
	unsigned int info_size;
	/* λͼ��ȣ���λ������ */
    unsigned int bmp_width;
	/* λͼ�߶ȣ���λ������ */
    unsigned int bmp_height;
	/* ɫ��ƽ������ֻ�� 1 Ϊ��Чֵ */
    unsigned short planes;
	/* ɫ���ÿ��������ռλ��(bits per pixel)����ֵΪ��1��4��8��16��24��32 */
    unsigned short bpp;
	/* ��ʹ�õ�ѹ��������һ����ֵΪ0������ѹ�� */
    unsigned int compression;
	/* ͼ���С��ָԭʼλͼ���ݵĴ�С��ǧ�����Ϊ���ļ���С */
    unsigned int bmp_size;
	/* ͼ��ĺ���ֱ��ʣ���λΪ����ÿ�ף�pixels-per-meter�� */
    unsigned int h_ppm;
	/* ͼ�������ֱ��ʣ���λΪ����ÿ�� */
    unsigned int v_ppm;
	/* ��ɫ�����ɫ������ֵΪ0ʱ��ʾ��ɫ��Ϊ2^ɫ��� */
    unsigned int colors;
	/* ��Ҫ��ɫ������ֵ������ɫ��ʱ�����ߵ���0ʱ������ʾ������ɫ��һ����Ҫ��ͨ������Ϊ0 */
    unsigned int i_colors;
}__attribute__((packed)) bmp_info;

/*********************************************************************
*����:		���BMPͼƬ���ļ�ͷ����Ϣͷ
*����:		
			width��BMPͼƬ�Ŀ��
			height��BMPͼƬ�ĸ߶�
*����ֵ:	�洢BMPͼƬ���ݵ��ڴ���׵�ַ
*********************************************************************/
static unsigned char *get_bmp_file_and_info(unsigned int width, unsigned int height)
{
    bmp_file file;
    bmp_info info;
	unsigned char *tmp = NULL;
	
	tmp = (unsigned char *)calloc(1, width*height*3+sizeof(file) + sizeof(info));
    bzero(&file, sizeof(file));
    bzero(&info, sizeof(info));
    file.flag = 0x4d42;	/* ��"BM" */
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
*����:		���BMPͼƬ����
*����:		
			dest���洢BMPͼƬ���ݵ��ڴ���׵�ַ
			rgb24��BMPͼƬԭʼλͼ����
			width��BMPͼƬ�Ŀ��
			height��BMPͼƬ�ĸ߶�
*����ֵ:	�洢BMPͼƬ���ݵ��ڴ���׵�ַ
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
			/* bmp�ļ��е����ݴ�ŷ������������ϣ��������ң�BGR��˳�� */
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
*����:		��BMPͼƬ�ļ������ļ��������򴴽��������������������
*����:		BMPͼƬ�ļ����ļ���
*����ֵ:	BMPͼƬ�ļ����ļ�������
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
*����:		��BMPͼƬ�ļ���д��ͼƬ������
*����:		
			bmp_fd��BMPͼƬ�ļ����ļ�������
			bmp_data��BMPͼƬ������
			width��BMPͼƬ�Ŀ��
			height��BMPͼƬ�ĸ߶�
*����ֵ:	��
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
*����:		ָ����ȼ��߶ȵ�24λRGBͼ�񱣴�ΪBMPͼƬ
*����:		
			rgb24��24λRGBͼ��
			bmp_name��BMPͼƬ������
			width��RGBͼ��Ŀ��
			height��RGBͼ��ĸ߶�
*����ֵ:	��
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
*����:		���ʱ���ǩ
*����:		
			buf���洢ʱ���ǩ���ڴ���׵�ַ
			length��ʱ���ǩ���ڴ�����ĳ���
*����ֵ:	�洢ʱ���ǩ���ڴ���׵�ַ
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
