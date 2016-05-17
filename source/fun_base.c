#include <stdio.h>
#include <time.h>
#include <string.h>

#include "TFT_API.h"


char getfocus(CUR_VAL ts_temp, short x, short y, short Width, short Height)
{	
	if((ts_temp.current_x >= x)
		&& (ts_temp.current_x <= (x+Width))
		&& (ts_temp.current_y >= y)
		&& (ts_temp.current_y <= (y+Height))
		&& (ts_temp.key == 0))//按键抬起或按下标志,1为按下,0为抬起,仅对单点起作用
	{
		return 1;
	}
	else
	{
		return 0;
	}
}
