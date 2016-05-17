#ifndef _TFT_API_H_
#define _TFT_API_H_

/**************************************************************/
/***************************������API**************************/
/**************************************************************/
#define MAX_FINGER_NUM		5		/* ��������Ĵ��ص�*/
#define ABS_MT_PRESSURE		0x3a	/* Pressure on contact area */
#define ABS_MT_TOUCH_MAJOR	0x30	/* Major axis of touching ellipse */

typedef struct Cur_val
{
	int current_x;		//������
	int current_y;		//������
	int current_p;		//ѹ��ֵ
	int touch_major;	//������ϵ( ��㴥��ʱ���ϱ�,��ѹ��ֵһ�����ݲ������ʲô��)
	int key;			//����̧����±�־,1Ϊ����,0Ϊ̧��,���Ե���������
	int state;			//������Ч״̬��־λ��0Ϊ������Ч��1Ϊ������Ч
}CUR_VAL;

//===================================================================================
//�򿪴������豸�ļ�
//===================================================================================
extern int open_touch_dev(const char *devname);

//===================================================================================
//��㴥����������
//===================================================================================
extern void analysis_event_mult(int touch_fd, CUR_VAL *val);

//===================================================================================
//���㴥����������
//===================================================================================
extern void analysis_event_single(int touch_fd, CUR_VAL *val);


/**************************************************************/
/*****************************LCD��****************************/
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
	short TLx;						// ��ͼ����ʼ����
	short TLy;						// ��ͼ����ʼ����
	short BRx;						// ��ͼ����������
	short BRy;						// ��ͼ����������
	short Width;					// ��ͼ�����
	short Height;					// ��ͼ���߶�
	short CurTextX;					// ���ֻ�������
	short CurTextY;					// ���ֻ�������
	unsigned short Transparency;	// ��ͼ͸����
	short Flag;						// ��Ч��־
	COLOR FGColor;					// ��ͼǰ��ɫ
	COLOR BGColor;					// ��ͼ����ɫ
	FONT AsciiFont;					// ��ǰʹ�õ�ASCII�ֿ�
	FONT ChineseFont;				// ��ǰʹ�õ������ֿ�
}STR_WINDOW;

typedef struct
{
	unsigned short Width;
	unsigned short Height;
	void *ImageBuf;
}STR_IMAGE;

//ͼ����ʾ��ɫ ת���궨��
#define RGB(r, g, b) RGB24(r, g, b)
#define RGB24(r, g, b) ((((COLOR)r)<<16) | (((COLOR)g)<<8) | ((COLOR)b))
#define RGB16(rgb) ((((COLOR)rgb&0xffe0)<<1) | ((COLOR)rgb&0x1f))

//�ض���ɫ RGB888
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

// PAINT_SOLID : ʵ��
#define	PAINT_SOLID		0

// PAINT_HOLLOW: ����
#define	PAINT_HOLLOW	1

#define	IMAGE_NO_SCALE	0
#define	IMAGE_SCALE		1

#define	NO_FREE_WIN -1

/*===========================================================*/
/*                      ���ڲ����ຯ��                       */
/*===========================================================*/

//=============================================================
//	TFT���������ʼ��
//	devname��TFT LCD�����豸�ļ��� 
//=============================================================
extern void TFT_Init(char *devname);

//=============================================================
//	������������
//=============================================================
extern WIN_HANDLE TFT_CreateWindowEx(short TLx,		// ���Ͻ�����
							  short TLy,
							  short width,			//���ڿ��
							  short height,			//���ڸ߶�
							  COLOR BGColor);		// ������ɫ

extern WIN_HANDLE TFT_CreateWindow(short TLx,		// ���Ͻ�����
							 short TLy,
							 short BRx,				// ���½�����
							 short BRy,
							 COLOR BGColor);		// ������ɫ
 
//=============================================================
//	�رչ�������
//=============================================================
extern void TFT_CloseWindow(WIN_HANDLE Handle);

//=============================================================
//	ɾ���������ڣ���������������գ�
//=============================================================
extern void TFT_DeleteWindow(WIN_HANDLE Handle);

//=============================================================
//	���ƻ��ƶ���������
//=============================================================
extern int TFT_MoveWindowEx(WIN_HANDLE Handle, short TLx, short TLy, int Cut);

//=============================================================
//	���Ʋ��ƶ��������ڣ�ͬʱ�ƶ��������ݣ�
//=============================================================
//extern int TFT_CopyWindow(WIN_HANDLE Handle, short TLx, short TLy);
#define TFT_CopyWindow(Handle, TLx, TLy) TFT_MoveWindowEx(Handle, TLx, TLy, 0)

//=============================================================
//	�ƶ��������ڣ�ͬʱ�ƶ��������ݣ�
//=============================================================
//extern int TFT_MoveWindow(WIN_HANDLE Handle, short TLx, short TLy);
#define TFT_MoveWindow(Handle, TLx, TLy)	TFT_MoveWindowEx(Handle, TLx, TLy, 1)

//=============================================================
//	���蹤�����ڣ����ı���Ļͼ��
//=============================================================
extern int TFT_ResetWindow(WIN_HANDLE Handle, short TLx, short TLy, short BRx, short BRy);

//=============================================================
//	�ƶ���������λ�ã����ı���Ļͼ��
//=============================================================
extern int TFT_MoveWindowBorder(WIN_HANDLE Handle, short TLx, short TLy);

//=============================================================
//	������������
//=============================================================
extern void TFT_ClearWindow(WIN_HANDLE Handle);

//=============================================================
//	��ȡ�������ڿ��
//=============================================================
extern short TFT_GetWindowWidth(WIN_HANDLE Handle);

//=============================================================
//	��ȡ�������ڸ߶�
//=============================================================
extern short TFT_GetWindowHeight(WIN_HANDLE Handle);

//=============================================================
//	��������ʾ���ݵ�͸����
//=============================================================
extern void TFT_SetTransparency(WIN_HANDLE Handle, unsigned short TransparencySet);

//=============================================================
//	��ȡ��ǰ�趨��͸����
//=============================================================
extern unsigned short TFT_GetTransparency(WIN_HANDLE Handle);

//=============================================================
//	������ʾ���ݵ�ǰ��ɫ
//=============================================================
extern void TFT_SetColor(WIN_HANDLE Handle, unsigned int ColorSet);

//=============================================================
//	��ȡ��ǰ�趨��ǰ��ɫ
//=============================================================
extern unsigned short TFT_GetColor(WIN_HANDLE Handle);

//=============================================================
//	�趨��ʾ����ɫ���Թ��������ı���Ч��
//=============================================================
extern void TFT_SetBGColor(WIN_HANDLE Handle, unsigned int ColorSet);

//=============================================================
//	��ȡ��ǰ�趨�ı���ɫ
//=============================================================
extern unsigned short TFT_GetBGColor(WIN_HANDLE Handle);

//=============================================================
//	���ù������е��ı���ʾλ��
//=============================================================
extern void TFT_SetTextPos(WIN_HANDLE Handle, short x, short y);

//=============================================================
//	��ȡ�������м�����ʾ���ı�λ��
//=============================================================
extern void TFT_GetTextPos(WIN_HANDLE Handle, short *x, short *y);

//=============================================================
//	���õ�ǰʹ�õ������ֿ�
//=============================================================
extern void TFT_SetChineseFont(WIN_HANDLE Handle, FONT FontID);

//=============================================================
//	��ȡ��ǰʹ�õ������ֿ���Ϣ
//=============================================================
extern FONT TFT_GetChineseFont(WIN_HANDLE Handle, STR_FONT *FontInfo);

//=============================================================
//	���õ�ǰʹ�õ�ASCII�ֿ�
//=============================================================
extern void TFT_SetAsciiFont(WIN_HANDLE Handle, FONT FontID);

//=============================================================
//	��ȡ��ǰʹ�õ�ASCII�ֿ���Ϣ
//=============================================================
extern FONT TFT_GetAsciiFont(WIN_HANDLE Handle, STR_FONT *FontInfo);

/*===========================================================*/
/*                        ��ͼ�ຯ��                         */
/*===========================================================*/

//=============================================================
//	�ڹ�������ָ��λ�û���
//=============================================================
extern void TFT_PutPixel(WIN_HANDLE Handle, short x, short y);

//=============================================================
//	��ȡ��������ָ���������ص���ɫ
//=============================================================
extern unsigned short TFT_PickColor(WIN_HANDLE Handle, short x, short y);

//=============================================================
//	����ʾ��������ָ��λ�û���
//=============================================================
extern void TFT_PutPixelAbsolute(WIN_HANDLE Handle, short AbsX, short AbsY);

//=============================================================
//	��ȡ��ʾ��������ָ���������ص���ɫ
//=============================================================
extern unsigned short TFT_PickColorAbsolute(WIN_HANDLE Handle, short AbsX, short AbsY);

//=============================================================
//	�ڹ������и�����ɫ�������
//=============================================================
extern void TFT_PutPixelLine(WIN_HANDLE Handle, short x, short y, short len, COLOR *ColorTab);

//=============================================================
//	�ڹ������л�ֱ��
//=============================================================
extern void TFT_Line(WIN_HANDLE Handle, short x1, short y1, short x2, short y2);

//=============================================================
//	�ڹ������л�Բ
//=============================================================
extern void TFT_Circle(WIN_HANDLE Handle, unsigned x,unsigned y,unsigned r,unsigned Mode);

//=============================================================
//	�ڹ������о���
//=============================================================
extern void TFT_Rectangle(WIN_HANDLE Handle,		// ����ָ��
				   unsigned x1,				// ���Ͻ�����
				   unsigned y1,
				   unsigned x2,				// ���½�����
				   unsigned y2,
				   unsigned Mode);			// PAINT_HOLLOW: ����
				   							// PAINT_SOLID : ʵ��

//=============================================================
//	��ʾһ���ַ�����
//=============================================================
extern void TFT_PutChar(WIN_HANDLE Handle, unsigned short CharCode);

//=============================================================
//	��ʾ�ַ���
//=============================================================
extern void TFT_PutString(WIN_HANDLE Handle, char *CharBuf);

//=============================================================
//	����ָ����ʽ��ӡ�ַ���
//=============================================================
extern void TFT_Print(WIN_HANDLE Handle, const char *format, ...);

//����TFT_Print��ӡ��Ϣ�ı���Ϊutf8,Ĭ��Ϊansi����
void set_tft_print_utf8(void);

//����TFT_Print��ӡ��Ϣ�ı���Ϊansi,Ĭ��Ϊansi����
void set_tft_print_ansi(void);

//=============================================================
//	�ڹ���������ʾͼ��
//=============================================================
extern void TFT_PutImage(WIN_HANDLE Handle, short x, short y, STR_IMAGE *pImage);
extern void TFT_PutImageEx(WIN_HANDLE Handle, short x, short y, short width, short height, void *pImage);

//=============================================================
//	�ڹ���������ʾͼ��
//=============================================================
extern void TFT_PutPicture(WIN_HANDLE Handle, short x, short y, unsigned char *pImage, int AutoScale);

//=============================================================
//	�ڹ���������ʾͼ��
//=============================================================
extern void TFT_File_Picture(WIN_HANDLE Handle, short x, short y, char *Bmp_Name, int AutoScale);

#endif
