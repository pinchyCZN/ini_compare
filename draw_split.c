#include <windows.h>
#include <commctrl.h>
#include "resource.h"

int draw_split(DRAWITEMSTRUCT *di,HWND hwnd)
{
	HWND hdc;
	RECT rect={0};
	static HBRUSH hbrush=0;
	HWND hlview1,hlview2;
	int i,height,count,start,end;
	if(0==di)
		return 0;
	hdc=di->hDC;
	if(0==hdc)
		return 0;
	if(0==hbrush)
		hbrush=CreateSolidBrush(RGB(0xFF,0,0));
	if(0==hbrush)
		return 0;

	hlview1=GetDlgItem(hwnd,IDC_LVIEW_LEFT);
	hlview2=GetDlgItem(hwnd,IDC_LVIEW_RIGHT);
	GetWindowRect(hlview1,&rect);
	height=rect.bottom-rect.top-GetSystemMetrics(SM_CYVSCROLL);
	count=ListView_GetItemCount(hlview1);
	start=end=-1;
	for(i=0;i<count;i++){
		LV_ITEM lvitem={0};
		int param=0;
		lvitem.mask=LVIF_PARAM;
		lvitem.iItem=i;
		ListView_GetItem(hlview1,&lvitem);
		param|=lvitem.lParam;
		lvitem.lParam=0;
		ListView_GetItem(hlview2,&lvitem);
		param|=lvitem.lParam;
		if(param){
			if(-1==start)
				start=i;
			if(i==(count-1))
				end=i+1;
		}else{
			if(start!=-1)
				end=i;
		}
		if(end!=-1){
			float y,len;
			len=height;
			len=len/count;
			len=len*(end-start);
			if(len<1)
				len=1;
			y=height;
			y=y/count;
			y=y*start;
			
			rect=di->rcItem;
			if(y>=rect.bottom)
				y=rect.bottom-1;
			rect.top=y;
			if((y+len)>rect.bottom)
				len=1;
			rect.bottom=y+len;
			FillRect(hdc,&rect,hbrush);

			start=end=-1;
		}

	}

	return 0;
}
int invalidate_split()
{
	extern HWND ghwnd;
	HWND hsplit;
	hsplit=GetDlgItem(ghwnd,IDC_SPLIT);
	if(hsplit!=0)
		InvalidateRect(hsplit,NULL,TRUE);
	return 0;
}