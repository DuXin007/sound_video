#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "jpeglib.h"
#include "jerror.h"

/* choose an efficiently fread'able size */
#define OUTPUT_BUF_SIZE 4096
#define INPUT_BUF_SIZE 4096

typedef struct{
	struct jpeg_source_mgr pub;	/* public fields */
	JOCTET *buffer;		/* start of buffer */
	char *inbuffer;
	int inbuffer_size;
	int inbuffer_cursor;
	int start_of_file;	/* have we gotten any data yet? */
}jpeg_source_mem;

typedef struct{
	struct jpeg_destination_mgr pub;/* public fields */
	JOCTET * buffer;				/* start of buffer */
	unsigned char *outbuffer;
	int outbuffer_size;
	unsigned char *outbuffer_cursor;
	int *written;
}jpeg_destination_mem;

typedef jpeg_source_mem * jpeg_src_mem_ptr;

typedef jpeg_destination_mem * jpeg_dest_mem_ptr;


static void init_source(j_decompress_ptr cinfo)
{
	jpeg_src_mem_ptr src = (jpeg_src_mem_ptr)cinfo->src;

	/* We reset the empty-input-file flag for each image,
	* but we don't clear the input buffer.
	* This is correct behavior for reading a series of images from one source.
	*/
	src->start_of_file = 1;
}

static int fill_input_buffer(j_decompress_ptr cinfo)
{
	jpeg_src_mem_ptr src = (jpeg_src_mem_ptr)cinfo->src;
	size_t nbytes;

	nbytes = src->inbuffer_size-src->inbuffer_cursor;
	if (nbytes>INPUT_BUF_SIZE)
		nbytes = INPUT_BUF_SIZE;

	if(nbytes <= 0){
		if(src->start_of_file)	/* Treat empty input file as fatal error */
			ERREXIT(cinfo, JERR_INPUT_EMPTY);
		WARNMS(cinfo, JWRN_JPEG_EOF);
		/* Insert a fake EOI marker */
		src->buffer[0] = (JOCTET) 0xFF;
		src->buffer[1] = (JOCTET) JPEG_EOI;
		nbytes = 2;
	}

	memcpy(src->buffer,src->inbuffer+src->inbuffer_cursor,nbytes);
	src->inbuffer_cursor+=nbytes;
	src->pub.next_input_byte = src->buffer;
	src->pub.bytes_in_buffer = nbytes;
	src->start_of_file = FALSE;
	return 1;
}

static void skip_input_data(j_decompress_ptr cinfo, long num_bytes)
{
	jpeg_src_mem_ptr src = (jpeg_src_mem_ptr) cinfo->src;

	/* Just a dumb implementation for now.  Could use fseek() except
	* it doesn't work on pipes.  Not clear that being smart is worth
	* any trouble anyway --- large skips are infrequent.
	*/
	if(num_bytes > 0){
		while(num_bytes > (long) src->pub.bytes_in_buffer){
			num_bytes -= (long) src->pub.bytes_in_buffer;
			fill_input_buffer(cinfo);
			/* note we assume that fill_input_buffer will never return FALSE,
			* so suspension need not be handled.
			*/
		}
		src->pub.next_input_byte += (size_t) num_bytes;
		src->pub.bytes_in_buffer -= (size_t) num_bytes;
	}
}

static void term_source (j_decompress_ptr cinfo)
{
  /* no work necessary here */
}

static void jpeg_memory_src(j_decompress_ptr cinfo,
			unsigned char *inbuffer,
			int inbuffer_size)
{
	jpeg_src_mem_ptr src;

	/* The source object and input buffer are made permanent so that a series
	* of JPEG images can be read from the same file by calling jpeg_stdio_src
	* only before the first one.  (If we discarded the buffer at the end of
	* one image, we'd likely lose the start of the next one.)
	* This makes it unsafe to use this manager and a different source
	* manager serially with the same JPEG object.  Caveat programmer.
	*/
	if(cinfo->src == NULL){	/* first time for this JPEG object? */
		cinfo->src = (struct jpeg_source_mgr *)(*cinfo->mem->alloc_small)((j_common_ptr) cinfo, JPOOL_PERMANENT, sizeof(jpeg_source_mem));
		src = (jpeg_src_mem_ptr)cinfo->src;
		src->buffer = (JOCTET *)(*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT, INPUT_BUF_SIZE * sizeof(JOCTET));
	}  
	src = (jpeg_src_mem_ptr)cinfo->src;
	src->pub.init_source = init_source;
	src->pub.fill_input_buffer = fill_input_buffer;
	src->pub.skip_input_data = skip_input_data;
	src->pub.resync_to_restart = jpeg_resync_to_restart; /* use default method */
	src->pub.term_source = term_source;
	src->inbuffer = (char *)inbuffer;
	src->inbuffer_size = inbuffer_size;
	src->inbuffer_cursor = 0;
	src->pub.bytes_in_buffer = 0; /* forces fill_input_buffer on first read */
	src->pub.next_input_byte = NULL; /* until buffer loaded */
}

static void init_destination(j_compress_ptr cinfo)
{
	jpeg_dest_mem_ptr dest = (jpeg_dest_mem_ptr) cinfo->dest;
	/* Allocate the output buffer --- it will be released when done with image */
	dest->buffer = (JOCTET *)(*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE, OUTPUT_BUF_SIZE * sizeof(JOCTET));
	*(dest->written) = 0;
	dest->pub.next_output_byte = dest->buffer;
	dest->pub.free_in_buffer = OUTPUT_BUF_SIZE;
}

/******************************************************************************
  Description.: called whenever local jpeg buffer fills up
  Input Value.: 
  Return Value: 
 ******************************************************************************/
static int empty_output_buffer(j_compress_ptr cinfo)
{
	jpeg_dest_mem_ptr dest = (jpeg_dest_mem_ptr) cinfo->dest;

	memcpy(dest->outbuffer_cursor, dest->buffer, OUTPUT_BUF_SIZE);
	dest->outbuffer_cursor += OUTPUT_BUF_SIZE;
	*(dest->written) += OUTPUT_BUF_SIZE;
	dest->pub.next_output_byte = dest->buffer;
	dest->pub.free_in_buffer = OUTPUT_BUF_SIZE;
	return 1;
}

/******************************************************************************
  Description.: called by jpeg_finish_compress after all data has been written.
  Usually needs to flush buffer.
  Input Value.: 
  Return Value: 
 ******************************************************************************/
static void term_destination(j_compress_ptr cinfo)
{
	jpeg_dest_mem_ptr dest = (jpeg_dest_mem_ptr) cinfo->dest;
	size_t datacount = OUTPUT_BUF_SIZE - dest->pub.free_in_buffer;
	/* Write any data remaining in the buffer */
	memcpy(dest->outbuffer_cursor, dest->buffer, datacount);
	dest->outbuffer_cursor += datacount;
	*(dest->written) += datacount;
}

static void jpeg_memory_dest(j_compress_ptr cinfo, unsigned char *buffer, int buffer_size, int *written)
{
	jpeg_dest_mem_ptr dest;

	if(cinfo->dest == NULL){
		cinfo->dest = (struct jpeg_destination_mgr *)(*cinfo->mem->alloc_small)((j_common_ptr) cinfo, JPOOL_PERMANENT, sizeof(jpeg_destination_mem));
	}
	dest = (jpeg_dest_mem_ptr)cinfo->dest;
	dest->pub.init_destination = init_destination;
	dest->pub.empty_output_buffer = empty_output_buffer;
	dest->pub.term_destination = term_destination;
	dest->outbuffer = buffer;
	dest->outbuffer_size = buffer_size;
	dest->outbuffer_cursor = buffer;
	dest->written = written;
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
static void yuv_to_rgb_pixel(unsigned char **pixel, int y, int u, int v, int flag)
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

	if(flag == 0){
		(*pixel)[0] = rgbdata[R];  
		(*pixel)[1] = rgbdata[G];  
		(*pixel)[2] = rgbdata[B];
	}else if(flag == 1){
		(*pixel)[0] = rgbdata[B];
		(*pixel)[1] = rgbdata[G];
		(*pixel)[2] = rgbdata[R];
	}
	(*pixel) +=3;
}

/*********************************************************************
*功能:		yuyv格式的图像转为rgb24格式的图像
*参数:		
			yuv：yuyv格式图像的首地址
			rgb24：rgb24格式图像的首地址，（颜色顺序为RGB），若为NULL，则不转换
			bgr24：bgr24格式图像的首地址，（颜色顺序为BGR），若为NULL，则不转换
			width：图像的宽度
			height：图像的高度
			flag：
				flag=0:将yuyv彩色图像转化为rgb彩色图像
				flag=1:将yuyv彩色图像转化为rgb黑白图像
*返回值:	无
*********************************************************************/
static void yuyv_to_rgb24(const void *yuyv,
		void **rgb24,
		void **bgr24,
		unsigned int width,
		unsigned int height,
		int flag)
{  
	int yuvdata[4];  
	unsigned char *yuv_buf;
	unsigned char *rgb_temp;
	unsigned char *bgr_temp; 
	unsigned int i, j;  

#define Y0  0  
#define U   1  
#define Y1  2  
#define V   3  

	yuv_buf = (unsigned char *)yuyv;
	if(rgb24 != NULL){
		rgb_temp = calloc(1, width * height * 3);
		*((unsigned char **)rgb24) = rgb_temp;
		for(i = 0; i < height * 2; i++){  
			for(j = 0; j < width; j+= 4){  
				/* get Y0 U Y1 V */  
				yuvdata[Y0] = *(yuv_buf + i * width + j + 0);  
				yuvdata[U]  = *(yuv_buf + i * width + j + 1);  
				yuvdata[Y1] = *(yuv_buf + i * width + j + 2);  
				yuvdata[V]  = *(yuv_buf + i * width + j + 3);  

				if(flag == 0){
					yuv_to_rgb_pixel(&rgb_temp, yuvdata[Y0], yuvdata[U], yuvdata[V], 0);
					yuv_to_rgb_pixel(&rgb_temp, yuvdata[Y1], yuvdata[U], yuvdata[V], 0);
				}else{
					yuv_to_rgb_pixel(&rgb_temp, yuvdata[Y0], 128, 128, 0);
					yuv_to_rgb_pixel(&rgb_temp, yuvdata[Y1], 128, 128, 0);
				}
			}
		}
	}
	if(bgr24 != NULL)
	{
		bgr_temp = calloc(1, width * height * 3);
		*((unsigned char **)bgr24) = bgr_temp;
		for(i = 0; i < height * 2; i++){  
			for(j = 0; j < width; j+= 4){  
				/* get Y0 U Y1 V */  
				yuvdata[Y0] = *(yuv_buf + i * width + j + 0);  
				yuvdata[U]  = *(yuv_buf + i * width + j + 1);  
				yuvdata[Y1] = *(yuv_buf + i * width + j + 2);  
				yuvdata[V]  = *(yuv_buf + i * width + j + 3);  

				if(flag == 0){
					yuv_to_rgb_pixel(&bgr_temp, yuvdata[Y0], yuvdata[U], yuvdata[V], 1);
					yuv_to_rgb_pixel(&bgr_temp, yuvdata[Y1], yuvdata[U], yuvdata[V], 1);
				}else{
					yuv_to_rgb_pixel(&bgr_temp, yuvdata[Y0], 128, 128, 1);
					yuv_to_rgb_pixel(&bgr_temp, yuvdata[Y1], 128, 128, 1);
				}
			}
		}
	} 
}

/*
一、压缩操作过程：
	1、分配JPEG解压对象空间，并初始化
	2、指定压缩数据的目标（例如，一个文件）
	3、为压缩设定参数，包括图像大小，颜色空间
	4、开始解压缩
	5、 取出数据
	6、解压缩完毕
	7、释放资源
*/

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
		int quality)
{
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;
	JSAMPROW row_pointer;	/* pointer to JSAMPLE row[s] */
	FILE *outfile = NULL;

	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);
	outfile = fopen(jpeg_filename, "wb");
	if(outfile == NULL){
		fprintf(stderr, "can't open %s\n", jpeg_filename);
		return -1;
	}
	jpeg_stdio_dest(&cinfo, outfile);
	cinfo.image_width = width;
	cinfo.image_height = height;
	cinfo.input_components = 3;
	cinfo.in_color_space = JCS_RGB;
	jpeg_set_defaults(&cinfo);
	jpeg_set_quality(&cinfo, quality, TRUE);
	jpeg_start_compress(&cinfo, TRUE);
	while(cinfo.next_scanline < cinfo.image_height){
		row_pointer = &rgb24[cinfo.next_scanline * width * 3];
		jpeg_write_scanlines(&cinfo, &row_pointer, 1);
	}
	jpeg_finish_compress(&cinfo);
	jpeg_destroy_compress(&cinfo);
	fclose(outfile);
	return 0;
}

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
		int quality)
{
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;
	JSAMPROW row_pointer;
	int written;
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress (&cinfo);
	jpeg_memory_dest(&cinfo, jpeg_buffer, buffer_size, &written);
	cinfo.image_width = width;
	cinfo.image_height = height;
	cinfo.input_components = 3;
	cinfo.in_color_space = JCS_RGB;
	//cinfo.in_color_space = JCS_GRAYSCALE; //JCS_GRAYSCALE表示灰度图，JCS_RGB表示彩色图像
	jpeg_set_defaults(&cinfo);
	jpeg_set_quality(&cinfo, quality, TRUE);
	jpeg_start_compress(&cinfo, TRUE);
	while(cinfo.next_scanline < height){
		row_pointer = rgb24 + width*3*cinfo.next_scanline;
		jpeg_write_scanlines(&cinfo, &row_pointer, 1);
	}
	jpeg_finish_compress(&cinfo);
	jpeg_destroy_compress(&cinfo);
	return written;
}

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
		int quality)
{
	unsigned char *rgb24 = NULL;
	int written;
	
	yuyv_to_rgb24(yuyv, (void *)&rgb24, NULL, width, height, 0);
	written = compress_rgb24_to_jpeg_buffer(rgb24, width, height, jpeg_buffer, buffer_size, quality);
	free(rgb24);
	return written;
}

/*
二、解压缩操作过程：
	1、分配JPEG解压对象空间，并初始化
	2、指定解压缩数据源（例如，一个文件）
	3、获取文件信息（调用jpeg_read_header 函数来获取图像信息）
	4、为解压缩设定参数，包括图像大小，颜色空间
	5、开始解压缩
	6、取出数据
	7、解压缩完毕
	8、释放资源
*/

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
		int *height)
{
	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;
	JSAMPROW row_pointer;
	int size = 0;

	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_decompress(&cinfo);
	/*
	FILE * infile;

	if ((infile = fopen("sample.jpg", "rb")) == NULL)
	{
		return 0;
	}
	jpeg_stdio_src(&cinfo, infile);
	*/
	jpeg_memory_src(&cinfo, jpeg_buffer, jpeg_size);
	jpeg_read_header(&cinfo, TRUE);
		
	(*width) = cinfo.image_width;
	(*height) = cinfo.image_height;
	size = cinfo.image_width*cinfo.image_height*cinfo.num_components;
	(*rgb) = (unsigned char *)calloc(1, size);
	
	/* 解压出来的图像大小为原图像的：scale_num/scale_denom。*/
	cinfo.scale_num=1;
	cinfo.scale_denom=1;
	cinfo.out_color_space = JCS_RGB;
	jpeg_start_decompress(&cinfo);
	while (cinfo.output_scanline < cinfo.output_height){
		row_pointer = (*rgb) + cinfo.output_scanline*cinfo.image_width*cinfo.num_components;
		jpeg_read_scanlines(&cinfo, &row_pointer, 1);
	}
	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);
	return size;
}

