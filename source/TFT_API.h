#ifndef _TFT_API_H_
#define _TFT_API_H_

/**************************************************************/
/***************************触摸屏API**************************/
/**************************************************************/
#define MAX_FINGER_NUM		5		/* 允许的最多的触控点*/
#define ABS_MT_PRESSURE		0x3a	/* Pressure on contact area */
#define ABS_MT_TOUCH_MAJOR	0x30	/* Major axis of touching ellipse */

typedef struct Cur_val
{
	int current_x;		//横坐标
	int current_y;		//纵坐标
	int current_p;		//压力值
	int touch_major;	//主坐标系( 多点触摸时会上报,跟压力值一样，暂不清楚有什么用)
	int key;			//按键抬起或按下标志,1为按下,0为抬起,仅对单点起作用
	int state;			//数据有效状态标志位：0为数据无效，1为数据有效
}CUR_VAL;

//===================================================================================
//打开触摸屏设备文件
//===================================================================================
extern int open_touch_dev(const char *devname);

//===================================================================================
//多点触摸分析程序
//===================================================================================
extern void analysis_event_mult(int touch_fd, CUR_VAL *val);

//===================================================================================
//单点触摸分析程序
//===================================================================================
extern void analysis_event_single(int touch_fd, CUR_VAL *val);


/**************************************************************/
/*****************************LCD屏****************************/
/**************************************************************/
typedef unsigned int TFT_INT;

typedef unsigned int COLOR;

typedef int WIN_HANDLE;

typedef short FONT;

typedef struct
{	
	unsigned short CharWidth;
	unsigned short CharHeight;
	unsigned char *FontBuf;
}STR_FONT;

typedef struct
{
	short TLx;						// 绘图区起始坐标
	short TLy;						// 绘图区起始坐标
	short BRx;						// 绘图区结束坐标
	short BRy;						// 绘图区结束坐标
	short Width;					// 绘图区宽度
	short Height;					// 绘图区高度
	short CurTextX;					// 文字绘制坐标
	short CurTextY;					// 文字绘制坐标
	unsigned short Transparency;	// 绘图透明度
	short Flag;						// 有效标志
	COLOR FGColor;					// 绘图前景色
	COLOR BGColor;					// 绘图背景色
	FONT AsciiFont;					// 当前使用的ASCII字库
	FONT ChineseFont;				// 当前使用的中文字库
}STR_WINDOW;

typedef struct
{
	unsigned short Width;
	unsigned short Height;
	void *ImageBuf;
}STR_IMAGE;

//图形显示颜色 转换宏定义
#define RGB(r, g, b) RGB24(r, g, b)
#define RGB24(r, g, b) ((((COLOR)r)<<16) | (((COLOR)g)<<8) | ((COLOR)b))
#define RGB16(rgb) ((((COLOR)rgb&0xffe0)<<1) | ((COLOR)rgb&0x1f))

//特定颜色 RGB888
#if TFT_COLOR_RESV
	#define COLOR_WHITE 	0x000000
	#define COLOR_BLACK		0xffffff
	#define COLOR_RED		0x00ffff
	#define COLOR_GREEN		0xff00ff
	#define COLOR_BLUE		0xffff00
	#define COLOR_YELLOW	0x00FFFF
#else
	#define COLOR_WHITE		0xffffff
	#define COLOR_BLACK		0x000000
	#define COLOR_RED		0xff0000
	#define COLOR_GREEN		0x00ff00
	#define COLOR_BLUE		0x0000ff
	#define COLOR_YELLOW	0xFFFF00
#endif

// PAINT_SOLID : 实心
#define	PAINT_SOLID		0

// PAINT_HOLLOW: 空心
#define	PAINT_HOLLOW	1

#define	IMAGE_NO_SCALE	0
#define	IMAGE_SCALE		1

#define	NO_FREE_WIN -1

/*===========================================================*/
/*                      窗口操作类函数                       */
/*===========================================================*/

//=============================================================
//	TFT驱动程序初始化
//	devname：TFT LCD驱动设备文件名 
//=============================================================
extern void TFT_Init(char *devname);

//=============================================================
//	创建工作窗口
//=============================================================
extern WIN_HANDLE TFT_CreateWindowEx(short TLx,		// 左上角坐标
							  short TLy,
							  short width,			//窗口宽度
							  short height,			//窗口高度
							  COLOR BGColor);		// 背景颜色

extern WIN_HANDLE TFT_CreateWindow(short TLx,		// 左上角坐标
							 short TLy,
							 short BRx,				// 右下角坐标
							 short BRy,
							 COLOR BGColor);		// 背景颜色
 
//=============================================================
//	关闭工作窗口
//=============================================================
extern void TFT_CloseWindow(WIN_HANDLE Handle);

//=============================================================
//	删除工作窗口（工作窗口区域清空）
//=============================================================
extern void TFT_DeleteWindow(WIN_HANDLE Handle);

//=============================================================
//	复制或移动工作窗口
//=============================================================
extern int TFT_MoveWindowEx(WIN_HANDLE Handle, short TLx, short TLy, int Cut);

//=============================================================
//	复制并移动工作窗口（同时移动窗体内容）
//=============================================================
//extern int TFT_CopyWindow(WIN_HANDLE Handle, short TLx, short TLy);
#define TFT_CopyWindow(Handle, TLx, TLy) TFT_MoveWindowEx(Handle, TLx, TLy, 0)

//=============================================================
//	移动工作窗口（同时移动窗体内容）
//=============================================================
//extern int TFT_MoveWindow(WIN_HANDLE Handle, short TLx, short TLy);
#define TFT_MoveWindow(Handle, TLx, TLy)	TFT_MoveWindowEx(Handle, TLx, TLy, 1)

//=============================================================
//	重设工作窗口（不改变屏幕图像）
//=============================================================
extern int TFT_ResetWindow(WIN_HANDLE Handle, short TLx, short TLy, short BRx, short BRy);

//=============================================================
//	移动工作窗口位置（不改变屏幕图像）
//=============================================================
extern int TFT_MoveWindowBorder(WIN_HANDLE Handle, short TLx, short TLy);

//=============================================================
//	工作窗口清屏
//=============================================================
extern void TFT_ClearWindow(WIN_HANDLE Handle);

//=============================================================
//	获取工作窗口宽度
//=============================================================
extern short TFT_GetWindowWidth(WIN_HANDLE Handle);

//=============================================================
//	获取工作窗口高度
//=============================================================
extern short TFT_GetWindowHeight(WIN_HANDLE Handle);

//=============================================================
//	设置新显示内容的透明度
//=============================================================
extern void TFT_SetTransparency(WIN_HANDLE Handle, unsigned short TransparencySet);

//=============================================================
//	获取当前设定的透明度
//=============================================================
extern unsigned short TFT_GetTransparency(WIN_HANDLE Handle);

//=============================================================
//	设置显示内容的前景色
//=============================================================
extern void TFT_SetColor(WIN_HANDLE Handle, unsigned int ColorSet);

//=============================================================
//	获取当前设定的前景色
//=============================================================
extern unsigned short TFT_GetColor(WIN_HANDLE Handle);

//=============================================================
//	设定显示背景色（对工作区、文本有效）
//=============================================================
extern void TFT_SetBGColor(WIN_HANDLE Handle, unsigned int ColorSet);

//=============================================================
//	获取当前设定的背景色
//=============================================================
extern unsigned short TFT_GetBGColor(WIN_HANDLE Handle);

//=============================================================
//	设置工作区中的文本显示位置
//=============================================================
extern void TFT_SetTextPos(WIN_HANDLE Handle, short x, short y);

//=============================================================
//	获取工作区中即将显示的文本位置
//=============================================================
extern void TFT_GetTextPos(WIN_HANDLE Handle, short *x, short *y);

//=============================================================
//	设置当前使用的中文字库
//=============================================================
extern void TFT_SetChineseFont(WIN_HANDLE Handle, FONT FontID);

//=============================================================
//	获取当前使用的中文字库信息
//=============================================================
extern FONT TFT_GetChineseFont(WIN_HANDLE Handle, STR_FONT *FontInfo);

//=============================================================
//	设置当前使用的ASCII字库
//=============================================================
extern void TFT_SetAsciiFont(WIN_HANDLE Handle, FONT FontID);

//=============================================================
//	获取当前使用的ASCII字库信息
//=============================================================
extern FONT TFT_GetAsciiFont(WIN_HANDLE Handle, STR_FONT *FontInfo);

/*===========================================================*/
/*                        绘图类函数                         */
/*===========================================================*/

//=============================================================
//	在工作区的指定位置画点
//=============================================================
extern void TFT_PutPixel(WIN_HANDLE Handle, short x, short y);

//=============================================================
//	获取工作区内指定坐标像素的颜色
//=============================================================
extern unsigned short TFT_PickColor(WIN_HANDLE Handle, short x, short y);

//=============================================================
//	在显示缓冲区的指定位置画点
//=============================================================
extern void TFT_PutPixelAbsolute(WIN_HANDLE Handle, short AbsX, short AbsY);

//=============================================================
//	获取显示缓冲区中指定坐标像素的颜色
//=============================================================
extern unsigned short TFT_PickColorAbsolute(WIN_HANDLE Handle, short AbsX, short AbsY);

//=============================================================
//	在工作区中根据颜色表绘制行
//=============================================================
extern void TFT_PutPixelLine(WIN_HANDLE Handle, short x, short y, short len, COLOR *ColorTab);

//=============================================================
//	在工作区中画直线
//=============================================================
extern void TFT_Line(WIN_HANDLE Handle, short x1, short y1, short x2, short y2);

//=============================================================
//	在工作区中画圆
//=============================================================
extern void TFT_Circle(WIN_HANDLE Handle, unsigned x,unsigned y,unsigned r,unsigned Mode);

//=============================================================
//	在工作区中矩形
//=============================================================
extern void TFT_Rectangle(WIN_HANDLE Handle,		// 窗体指针
				   unsigned x1,				// 左上角坐标
				   unsigned y1,
				   unsigned x2,				// 右下角坐标
				   unsigned y2,
				   unsigned Mode);			// PAINT_HOLLOW: 空心
				   							// PAINT_SOLID : 实心

//=============================================================
//	显示一个字符或汉字
//=============================================================
extern void TFT_PutChar(WIN_HANDLE Handle, unsigned short CharCode);

//=============================================================
//	显示字符串
//=============================================================
extern void TFT_PutString(WIN_HANDLE Handle, char *CharBuf);

//=============================================================
//	根据指定格式打印字符串
//=============================================================
extern void TFT_Print(WIN_HANDLE Handle, const char *format, ...);

//设置TFT_Print打印信息的编码为utf8,默认为ansi编码
void set_tft_print_utf8(void);

//设置TFT_Print打印信息的编码为ansi,默认为ansi编码
void set_tft_print_ansi(void);

//=============================================================
//	在工作区中显示图像
//=============================================================
extern void TFT_PutImage(WIN_HANDLE Handle, short x, short y, STR_IMAGE *pImage);
extern void TFT_PutImageEx(WIN_HANDLE Handle, short x, short y, short width, short height, void *pImage);

//=============================================================
//	在工作区中显示图像
//=============================================================
extern void TFT_PutPicture(WIN_HANDLE Handle, short x, short y, unsigned char *pImage, int AutoScale);

//=============================================================
//	在工作区中显示图像
//=============================================================
extern void TFT_File_Picture(WIN_HANDLE Handle, short x, short y, char *Bmp_Name, int AutoScale);

#endif
