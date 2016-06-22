#if WINVER<0x500
	#define _WIN32_WINNT 0x500
#endif
#include <windows.h>
#include <Commctrl.h>

int get_first_line_len(const char *str)
{
	int i,len;
	len=0x10000;
	for(i=0;i<len;i++){
		if(str[i]=='\n' || str[i]=='\r' || str[i]==0)
			break;
	}
	return i;
}
int get_string_width_wc(HWND hwnd,const char *str,int wide_char)
{
	if(hwnd!=0 && str!=0){
		SIZE size={0};
		HDC hdc;
		hdc=GetDC(hwnd);
		if(hdc!=0){
			HFONT hfont;
			int len=get_first_line_len(str);
			hfont=SendMessage(hwnd,WM_GETFONT,0,0);
			if(hfont!=0){
				HGDIOBJ hold=0;
				hold=SelectObject(hdc,hfont);
				if(wide_char)
					GetTextExtentPoint32W(hdc,str,wcslen(str),&size);
				else
					GetTextExtentPoint32(hdc,str,len,&size);
				if(hold!=0)
					SelectObject(hdc,hold);
			}
			else{
				if(wide_char)
					GetTextExtentPoint32W(hdc,str,wcslen(str),&size);
				else
					GetTextExtentPoint32(hdc,str,len,&size);
			}
			ReleaseDC(hwnd,hdc);
			return size.cx;
		}
	}
	return 0;

}
int get_str_width(HWND hwnd,const char *str)
{
	return get_string_width_wc(hwnd,str,FALSE);
}
int lv_scroll_column(HWND hlistview,int index)
{
	RECT rect={0};
	int result=FALSE;
	if(lv_get_col_rect(hlistview,index,&rect)){
		RECT rectlv={0};
		SCROLLINFO si;
		si.cbSize=sizeof(si);
		si.fMask=SIF_POS;
		if(	GetClientRect(hlistview,&rectlv) &&
			GetScrollInfo(hlistview,SB_HORZ,&si)){
			int diff=0;
			if(rect.right-si.nPos>rectlv.right)
				diff=rect.right-rectlv.right-si.nPos;
			else if(rect.left-si.nPos<rectlv.left)
				diff=-si.nPos+rect.left;
			if(diff!=0)
				result=ListView_Scroll(hlistview,diff,0);
		}
	}
	return result;
}
int lv_get_col_rect(HWND hlistview,int col,RECT *rect)
{
	int result=FALSE;
	if(hlistview!=0 && rect!=0){
		HWND header=SendMessage(hlistview,LVM_GETHEADER,0,0);
		if(header!=0)
			if(SendMessage(header,HDM_GETITEMRECT,col,rect))
				result=TRUE;
	}
	return result;
}
int lv_get_column_count(HWND hlistview)
{
	HWND header;
	int count=0;
	header=SendMessage(hlistview,LVM_GETHEADER,0,0);
	if(header!=0){
		count=SendMessage(header,HDM_GETITEMCOUNT,0,0);
		if(count<0)
			count=0;
	}
	return count;
}
int lv_get_col_text(HWND hlistview,int index,char *str,int size)
{
	LV_COLUMN col;
	if(hlistview!=0 && str!=0 && size>0){
		col.mask = LVCF_TEXT;
		col.pszText = str;
		col.cchTextMax = size;
		return ListView_GetColumn(hlistview,index,&col);
	}
	return FALSE;
}
int lv_add_column(HWND hlistview,char *str,int index)
{
	LV_COLUMN col;
	if(hlistview!=0 && str!=0){
		HWND header;
		int width=0;
		header=SendMessage(hlistview,LVM_GETHEADER,0,0);
		width=get_str_width(header,str);
		width+=14;
		if(width<40)
			width=40;
		col.mask = LVCF_WIDTH|LVCF_TEXT;
		col.cx = width;
		col.pszText = str;
		if(ListView_InsertColumn(hlistview,index,&col)>=0)
			return width;
	}
	return 0;
}
int lv_update_data(HWND hlistview,int row,int col,const char *str)
{
	if(hlistview!=0 && str!=0){
		LV_ITEM item;
		memset(&item,0,sizeof(item));
		item.mask=LVIF_TEXT;
		item.iItem=row;
		item.pszText=str;
		item.iSubItem=col;
		return ListView_SetItem(hlistview,&item);
	}
	return FALSE;
}
int lv_insert_data(HWND hlistview,int row,int col,const char *str)
{
	if(hlistview!=0 && str!=0){
		LV_ITEM item;
		memset(&item,0,sizeof(item));
		if(col==0){
			item.mask=LVIF_TEXT|LVIF_PARAM;
			item.iItem=row;
			item.pszText=str;
			item.lParam=row;
			ListView_InsertItem(hlistview,&item);
		}
		else{
			item.mask=LVIF_TEXT;
			item.iItem=row;
			item.pszText=str;
			item.iSubItem=col;
			ListView_SetItem(hlistview,&item);
		}
		return TRUE;
	}
	return FALSE;
}

int reduce_color(int color)
{
	unsigned char r,g,b;
	//BGR
	r=color&0xFF;
	g=(color>>8)&0xFF;
	b=(color>>16)&0xFF;
	r/=3;
	g/=3;
	b/=3;
	color=(b<<16)|(g<<8)|r;
	return color;
}

int draw_item(DRAWITEMSTRUCT *di)
{
	int i,count,xpos;
	int textcolor,bgcolor;
	RECT bound_rect={0},client_rect={0};

	count=lv_get_column_count(di->hwndItem);
	if(count>1000)
		count=1000;

	GetClientRect(di->hwndItem,&client_rect);

	bgcolor=GetSysColor(di->itemState&ODS_SELECTED ? COLOR_HIGHLIGHT:COLOR_WINDOW);
	textcolor=GetSysColor(di->itemState&ODS_SELECTED ? COLOR_HIGHLIGHTTEXT:COLOR_WINDOWTEXT);
	xpos=0;
	for(i=0;i<count;i++){
		LV_ITEM lvi={0};
		RECT rect;
		int width,style;

		width=ListView_GetColumnWidth(di->hwndItem,i);

		rect=di->rcItem;
		rect.left+=xpos;
		rect.right=rect.left+width;
		if(rect.right<0)
			i=i;
		if(rect.left>client_rect.right)
			i=i;
		xpos+=width;


		if(rect.right>=0 && rect.left<=client_rect.right){
			char text[256]={0};
			lvi.mask=LVIF_TEXT|LVIF_PARAM;
			lvi.iItem=di->itemID;
			lvi.iSubItem=i;
			lvi.pszText=text;
			lvi.cchTextMax=sizeof(text);
			lvi.lParam=0;

			ListView_GetItem(di->hwndItem,&lvi);
			text[sizeof(text)-1]=0;

			FillRect(di->hDC,&rect,di->itemState&ODS_SELECTED ? COLOR_HIGHLIGHT+1:COLOR_WINDOW+1);

			if(text[0]!=0){
				int left_justify=1;
				if(lvi.lParam!=0)
					SetTextColor(di->hDC,0xFF);
				else
					SetTextColor(di->hDC,textcolor);
				SetBkColor(di->hDC,bgcolor);

				if(left_justify)
					style=DT_LEFT|DT_NOPREFIX;
				else
					style=DT_RIGHT|DT_NOPREFIX;

				DrawText(di->hDC,text,-1,&rect,style);
			}
		}
	}
	if(di->itemState&ODS_FOCUS){
		RECT rect=di->rcItem;
		rect.bottom--;
		rect.right--;
		FrameRect(di->hDC,&rect,GetStockObject(GRAY_BRUSH));
	}
	return TRUE;
}
int get_item_at_pt(HWND hlview,int x,int y,int *index)
{
	int result=FALSE;
	LV_HITTESTINFO lvhti={0};
	lvhti.pt.x=x;
	lvhti.pt.y=y;
	ListView_HitTest(hlview,&lvhti);
	if(!(lvhti.flags&LVHT_NOWHERE)){
		result=TRUE;
		*index=lvhti.iItem;
	}
	return result;
}
int seek_next_diff(HWND hlview,int vk_dir)
{
	int i,count,start=0,found=-1;
	LV_FINDINFO lvfi={0};
	POINT pt={0};
	count=ListView_GetItemCount(hlview);
	for(i=0;i<count;i++){
		int state;
		state=ListView_GetItemState(hlview,i,LVIS_FOCUSED,LVIS_FOCUSED);
		if(state&LVIS_FOCUSED){
			start=i;
			break;
		}
	}
	if(vk_dir==VK_UP){
		for(i=start-1;i>=0;i--){
			LV_ITEM lvi={0};
			lvi.mask=LVIF_PARAM;
			lvi.iItem=i;
			ListView_GetItem(hlview,&lvi);
			if(lvi.lParam!=0){
				found=i;
				break;
			}
		}
	}else{
		for(i=start+1;i<count;i++){
			LV_ITEM lvi={0};
			lvi.mask=LVIF_PARAM;
			lvi.iItem=i;
			ListView_GetItem(hlview,&lvi);
			if(lvi.lParam!=0){
				found=i;
				break;
			}
		}
	}
	if(found>=0){
		ListView_EnsureVisible(hlview,found,FALSE);
		ListView_SetItemState(hlview,found,LVIS_FOCUSED,LVIS_FOCUSED);
	}
	return found>=0;
}
extern HWND ghlvleft,ghlvright;
static WNDPROC wporiglistview=0;
LRESULT APIENTRY sc_listview(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
{
	static int ignore=0;
	static int start_item=0;
	//if(msg==WM_MOUSEWHEEL)
	//if(msg>=0x1000)
	//print_msg(msg,lparam,wparam,(int)hwnd);
	switch(msg){
	case WM_APP:
		if(lparam==0){
			HWND htmp;
			int index,deltay;
			RECT rect={0},r2={0};
			if(ignore)
				break;
			if(hwnd==ghlvleft)
				htmp=ghlvright;
			else
				htmp=ghlvleft;
			ignore=1;
			index=ListView_GetTopIndex(hwnd);
			ListView_GetItemRect(hwnd,index,&rect,LVIR_BOUNDS);

			ListView_GetItemRect(htmp,index,&r2,LVIR_BOUNDS);
			deltay=r2.top-rect.top;
			ListView_Scroll(htmp,0,deltay);
			ignore=0;
		}else{
			HWND htmp;
			int index,deltax;
			RECT rect={0},r2={0};
			if(ignore)
				break;
			if(hwnd==ghlvleft)
				htmp=ghlvright;
			else
				htmp=ghlvleft;
			ignore=1;
			index=ListView_GetTopIndex(hwnd);
			ListView_GetItemRect(hwnd,index,&rect,LVIR_BOUNDS);

			ListView_GetItemRect(htmp,index,&r2,LVIR_BOUNDS);
			deltax=r2.left-rect.left;
			ListView_Scroll(htmp,deltax,0);
			ignore=0;
		}
		return 0;
		break;
	case WM_MOUSEMOVE:
		{
			if(wparam==MK_LBUTTON){
				int index=0,x,y;
				x=LOWORD(lparam);
				y=HIWORD(lparam);
				if(get_item_at_pt(hwnd,x,y,&index)){
					int i,count,start,end;
					count=ListView_GetItemCount(hwnd);
					start=min(index,start_item);
					end=max(index,start_item);
					for(i=0;i<count;i++){
						int state=0;
						if(i>=start && i<=end)
							state=LVIS_SELECTED;
						ListView_SetItemState(hwnd,i,state,LVIS_SELECTED);
					}
				}
			}
		}
		break;
	case WM_SYSKEYDOWN:
		switch(wparam){
		case VK_UP:
		case VK_DOWN:
			seek_next_diff(hwnd,wparam);
			break;
		}
		break;
	case WM_KEYDOWN:
		switch(wparam){
		case VK_LEFT:
		case VK_RIGHT:
			PostMessage(hwnd,WM_APP,0,1);
			break;
		case VK_UP:
		case VK_DOWN:
		case VK_NEXT:
		case VK_PRIOR:
		case VK_HOME:
		case VK_END:
			PostMessage(hwnd,WM_APP,0,0);
			break;
		case VK_F5:
			PostMessage(GetParent(hwnd),WM_APP,0,0);
			break;
		case 'A':
			{
				int i,count;
				if(GetKeyState(VK_CONTROL)&0x8000){
					count=ListView_GetItemCount(hwnd);
					for(i=0;i<count;i++){
						ListView_SetItemState(hwnd,i,LVIS_SELECTED,LVIS_SELECTED);
					}
				}
			}
			break;
		}
		break;
	case WM_LBUTTONDOWN:
		{
			int i,count;
			HWND htmp;
			get_item_at_pt(hwnd,LOWORD(lparam),HIWORD(lparam),&start_item);
			if(hwnd==ghlvleft)
				htmp=ghlvright;
			else
				htmp=ghlvleft;
			count=ListView_GetItemCount(htmp);
			for(i=0;i<count;i++){
				ListView_SetItemState(htmp,i,0,LVIS_SELECTED);
			}
			SetFocus(hwnd);
		}
		break;
	case WM_MOUSEWHEEL:
		if(LOWORD(wparam)==MK_CONTROL){
			SHORT delta=HIWORD(wparam);
			int dy=delta/5;
			ListView_Scroll(hwnd,dy,0);
			PostMessage(hwnd,WM_APP,0,1);
			break;
		}
	case WM_VSCROLL:
		PostMessage(hwnd,WM_APP,0,0);
		break;
	case WM_HSCROLL:
		PostMessage(hwnd,WM_APP,0,1);
		break;
	}
    return CallWindowProc(wporiglistview,hwnd,msg,wparam,lparam);
}
int subclass_listview(HWND hlistview)
{
	wporiglistview=SetWindowLong(hlistview,GWL_WNDPROC,(LONG)sc_listview);
	return wporiglistview;
}

int init_listview(HWND hlview)
{
	const char *cols[]={"SECTION","KEY","DATA"};
	int i;
	for(i=0;i<sizeof(cols)/sizeof(cols[0]);i++){
		lv_add_column(hlview,cols[i],i);
	}
}