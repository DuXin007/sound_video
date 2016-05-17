#ifndef __YUYV_RGB_JPEG_H__
#define __YUYV_RGB_JPEG_H__

/**************************************************************************
 * 功能：	将rgb24的原始位图保存为jpeg图片
 * 参数：
			rgb24：rgb24的原始位图数据
			jpeg_filename：jpeg图片名
			width：rgb24的原始位图的宽度
			height：rgb24的原始位图的高度
 * 返回值:	成功：0；失败：-1
 ***************************************************************************/
int compress_rgb24_to_jpeg_file(unsigned char *rgb24,
		char *jpeg_filename,
		int width,
		int height,
		int quality);

/**************************************************************************
 * 功能：	将rgb24的原始位图保存为jpeg图片，并将图片的内容存放在内存中
 * 参数：
			rgb24：rgb24的原始位图数据
			width：rgb24的原始位图的宽度
			height：rgb24的原始位图的高度
			jpeg_buffer：存放jpeg图片的内存区域的首地址
			buffer_size：存放jpeg图片的内存区域的大小(一般设置为width*height*2)
			quality：将rgb24的原始位图保存为jpeg图片的质量
 * 返回值:	jpeg图片内容实际的大小
 ***************************************************************************/
int compress_rgb24_to_jpeg_buffer(unsigned char *rgb24,
		unsigned int width,
		unsigned int height,
		unsigned char *jpeg_buffer,
		int buffer_size,
		int quality);

/**************************************************************************
 * 功能：	yuyv格式的图像保存为jpeg图片，并将图片的内容存放在内存中
 * 参数：
			yuyv：yuyv格式的图像
			width：yuyv格式的图像的宽度
			height：yuyv格式的图像的高度
			jpeg_buffer：存放jpeg图片的内存区域的首地址
			buffer_size：存放jpeg图片的内存区域的大小(一般设置为width*height*2)
			quality：将rgb24的原始位图保存为jpeg图片的质量
 * 返回值:	jpeg图片内容实际的大小
 ***************************************************************************/
int compress_yuyv_to_jpeg_buffer(const void *yuyv,
		unsigned int width,
		unsigned int height,
		unsigned char *jpeg_buffer,
		int buffer_size,
		int quality);

/**************************************************************************
 * 功能：	将jpeg图片解压成rgb数据
 * 参数：
			jpeg_buffer：存放jpeg图片的内存区域的首地址
			jpeg_size：存放jpeg图片的内存区域的大小
			rgb：存放rgb格式图像首地址的指针的地址
			width：rgb格式的图像的宽度
			height：rgb格式的图像的高度
 * 返回值:	rgb格式图像的大小
 ***************************************************************************/
int decompress_jpeg_buffer_to_rgb(unsigned char *jpeg_buffer,
		int jpeg_size,
		unsigned char **rgb,
		int *width,
		int *height);

#endif				
