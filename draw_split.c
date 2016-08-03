#include <windows.h>

int draw_split(DRAWITEMSTRUCT *di)
{
	HWND hdc;
	RECT rect;
	static HBRUSH hbrush=0;
	int color;
	if(0==di)
		return 0;
	hdc=di->hDC;
	if(0==hdc)
		return 0;
	if(0==hbrush)
		hbrush=CreateSolidBrush(RGB(0xFF,0,0));
	rect=di->rcItem;
	FillRect(hdc,&rect,COLOR_BTNFACE+1);
	if(0==hbrush)
		return 0;
	{
	}
	return 0;
}