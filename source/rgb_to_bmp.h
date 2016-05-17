#ifndef _RGB_TO_BMP_
#define _RGB_TO_BMP_

/*********************************************************************
*功能:		指定宽度及高度的24位RGB图像保存为BMP图片
*参数:		
			rgb24：24位RGB图像
			bmp_name：BMP图片的名字
			width：RGB图像的宽度
			height：RGB图像的高度
*返回值:	无
*********************************************************************/
void rgb24_to_bmp(unsigned char *rgb24, char *bmp_name, int width, int height);

/*********************************************************************
*功能:		获得时间标签
*参数:		
			buf：存储时间标签的内存的首地址
			length：时间标签的内存区域的长度
*返回值:	存储时间标签的内存的首地址
*********************************************************************/
char *create_time_tag(char *buf, int length);

#endif
