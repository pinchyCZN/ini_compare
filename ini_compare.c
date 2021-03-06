#if WINVER<0x500
	#define _WIN32_WINNT 0x500
#endif
#include <windows.h>
#include <Commctrl.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include "resource.h"

HINSTANCE ghinstance=0;
int top_ypos=0;
HWND ghlvleft=0,ghlvright=0,ghwnd=0;
static HWND ghttip=0;
char fname_left[MAX_PATH]={0};
char fname_right[MAX_PATH]={0};
int left_dirty=FALSE;
int right_dirty=FALSE;
int timer_event=0;
#define TIMER_ID 1337
#define LVIEW_GAP 8

int update_left_fname(const char *s)
{
	strncpy(fname_left,s,sizeof(fname_left));
	fname_left[sizeof(fname_left)-1]=0;
	return TRUE;
}
int update_right_fname(const char *s)
{
	strncpy(fname_right,s,sizeof(fname_right));
	fname_right[sizeof(fname_right)-1]=0;
	return TRUE;
}
void open_console()
{
	char title[MAX_PATH]={0};
	HWND hcon;
	FILE *hf;
	static BYTE consolecreated=FALSE;
	static int hcrt=0;

	if(consolecreated==TRUE)
	{
		GetConsoleTitle(title,sizeof(title));
		if(title[0]!=0){
			hcon=FindWindow(NULL,title);
			ShowWindow(hcon,SW_SHOW);
		}
		hcon=(HWND)GetStdHandle(STD_INPUT_HANDLE);
		FlushConsoleInputBuffer(hcon);
		return;
	}
	AllocConsole();
	hcrt=_open_osfhandle((long)GetStdHandle(STD_OUTPUT_HANDLE),_O_TEXT);

	fflush(stdin);
	hf=_fdopen(hcrt,"w");
	*stdout=*hf;
	setvbuf(stdout,NULL,_IONBF,0);
	GetConsoleTitle(title,sizeof(title));
	if(title[0]!=0){
		hcon=FindWindow(NULL,title);
		ShowWindow(hcon,SW_SHOW);
		SetForegroundWindow(hcon);
	}
	consolecreated=TRUE;
}
int move_console(int x,int y)
{
	char title[MAX_PATH]={0};
	HWND hcon;
	GetConsoleTitle(title,sizeof(title));
	if(title[0]!=0){
		hcon=FindWindow(NULL,title);
		SetWindowPos(hcon,0,x,y,0,0,SWP_NOZORDER|SWP_NOSIZE);
	}
	return 0;
}
int setup_window(HWND hwnd,HINSTANCE hinst)
{
	int i,id_list[]={IDC_LVIEW_LEFT,IDC_LVIEW_RIGHT};
	RECT rect={0};
	GetWindowRect(GetDlgItem(hwnd,IDC_OPEN_LEFT),&rect);
	MapWindowPoints(NULL,hwnd,&rect,2);
	top_ypos=rect.bottom+5;
	for(i=0;i<sizeof(id_list)/sizeof(id_list[0]);i++){
		HWND hlview;
		int idc=id_list[i];
		DestroyWindow(GetDlgItem(hwnd,idc));
		hlview=CreateWindow(WC_LISTVIEW,"",
			WS_TABSTOP|WS_CHILD|WS_CLIPSIBLINGS|WS_VISIBLE|LVS_REPORT|LVS_SHOWSELALWAYS|LVS_OWNERDRAWFIXED,
			0,top_ypos,
			100,100,
			hwnd,
			idc,
			hinst,
			NULL);
		if(hlview!=0)
			ListView_SetExtendedListViewStyle(hlview,LVS_EX_GRIDLINES|LVS_EX_FULLROWSELECT);
	}
	return 0;
}
int resize_lview(HWND hwnd,int idc_left,int idc_right,int center)
{
	RECT rect={0};
	int x,y,w,h;
	HWND hleft,hright;
	GetClientRect(hwnd,&rect);
	hleft=GetDlgItem(hwnd,idc_left);
	hright=GetDlgItem(hwnd,idc_right);
	h=rect.bottom-top_ypos;
	SetWindowPos(hleft,HWND_TOP,0,0,center,h,SWP_NOMOVE|SWP_NOZORDER);
	x=center+LVIEW_GAP;
	y=top_ypos;
	w=rect.right-center-LVIEW_GAP;
	if(w<=10)
		w=10;
	h=rect.bottom-top_ypos;
	SetWindowPos(hright,HWND_TOP,x,y,w,h,SWP_NOZORDER);
	return 0;
}
int center_split(HWND hwnd,int center)
{
	RECT rect={0};
	HWND hbutton;
	int x,y,w,h;
	GetClientRect(hwnd,&rect);
	hbutton=GetDlgItem(hwnd,IDC_SPLIT);
	if(hbutton==0)
		return 0;
	x=center;
	y=top_ypos+GetSystemMetrics(SM_CYVSCROLL);
	w=LVIEW_GAP;
	h=rect.bottom-y;
	SetWindowPos(hbutton,NULL,x,y,w,h,SWP_NOZORDER);
	return 0;
}
int move_buttons(HWND hwnd,int center)
{
	RECT rect={0};
	int i,y,width;
	struct CTRL_OFFSETS{
		int idc;
		int pos;
	};
	struct CTRL_OFFSETS offsets[]={
		{IDC_OPEN_LEFT,-3},
		{IDC_SAVE_LEFT,-2},
		{IDC_TO_RIGHT,-1},
		{IDC_TO_LEFT,0},
		{IDC_SAVE_RIGHT,1},
		{IDC_OPEN_RIGHT,2},
		{IDC_CASE_SENSE,3},
		{IDC_SHOW_SPLIT,4}
	};
	center_split(hwnd,center);
	GetWindowRect(GetDlgItem(hwnd,IDC_OPEN_LEFT),&rect);
	MapWindowPoints(NULL,hwnd,&rect,2);
	y=rect.top;
	width=rect.right-rect.left;
	for(i=0;i<sizeof(offsets)/sizeof(offsets[0]);i++){
		HWND h=GetDlgItem(hwnd,offsets[i].idc);
		int x=center;
		x+=offsets[i].pos*(width+10);
		if(IDC_SHOW_SPLIT==offsets[i].idc)
			x+=40;
		SetWindowPos(h,NULL,x,y,0,0,SWP_NOSIZE|SWP_NOZORDER);
		InvalidateRect(h,0,FALSE);
	}
	return 0;
}

int get_center(HWND hwnd)
{
	RECT rect={0};
	GetClientRect(hwnd,&rect);
	return (rect.right-rect.left)/2;
}
int set_window_title(const char *f1,int changed1,const char *f2,int changed2)
{
#define MAX_NAME_LEN 60
	char tmp[256]={0};
	int l1,l2;
	char *p1,*p2;
	l1=strlen(f1);
	l2=strlen(f2);
	p1=f1;
	p2=f2;
	if(l1>MAX_NAME_LEN)
		p1=f1+l1-MAX_NAME_LEN;
	if(l2>MAX_NAME_LEN)
		p2=f2+l2-MAX_NAME_LEN;
	_snprintf(tmp,sizeof(tmp),"%s%s%s <-> %s%s%s",
		l1>MAX_NAME_LEN?"...":"",p1,changed1?"*":"",
		l2>MAX_NAME_LEN?"...":"",p2,changed2?"*":"");
	tmp[sizeof(tmp)-1]=0;
	return SetWindowText(ghwnd,tmp);
}
int open_file(char *fout,int flen,int left)
{
	char tmp[40]={0};
	OPENFILENAME ofn={0};
	ofn.lStructSize=sizeof(ofn);
	ofn.lpstrFilter=TEXT("*.INI\0*.INI\0*.*\0*.*\0\0");
	ofn.lpstrFile=fout;
	ofn.nMaxFile=flen;
	_snprintf(tmp,sizeof(tmp),"OPEN %s INI FILE",left?"LEFT":"RIGHT");
	ofn.lpstrTitle=tmp;
	ofn.Flags=OFN_ENABLESIZING;
	return GetOpenFileName(&ofn);
}
int move_data(HWND hsrc,HWND hdest)
{
	int i,count;
	count=ListView_GetItemCount(hsrc);
	for(i=0;i<count;i++){
		int state;
		state=ListView_GetItemState(hsrc,i,LVIS_SELECTED);
		if(state){
			char tmp[256]={0};
			ListView_GetItemText(hsrc,i,0,tmp,sizeof(tmp));
			if(tmp[0]!=0){
				char data[256]={0};
				LV_ITEM lvi={0};
				ListView_SetItemText(hdest,i,0,tmp);
				tmp[0]=0;
				ListView_GetItemText(hsrc,i,1,tmp,sizeof(tmp));
				ListView_GetItemText(hsrc,i,2,data,sizeof(data));
				ListView_SetItemText(hdest,i,1,tmp);
				ListView_SetItemText(hdest,i,2,data);
				lvi.mask=LVIF_PARAM;
				lvi.iItem=i;
				lvi.lParam=0;
				ListView_SetItem(hsrc,&lvi);
				ListView_SetItem(hdest,&lvi);
			}
		}
	}
	invalidate_split();
	return 0;
}
int save_data(HWND hlview,char *fname)
{
	int i,count,result=0;
	if(fname[0]==0)
		return result;
	count=ListView_GetItemCount(hlview);
	for(i=0;i<count;i++){
		char sect[80]={0};
		ListView_GetItemText(hlview,i,0,sect,sizeof(sect));
		if(sect[0]!=0){
			char key[80]={0};
			ListView_GetItemText(hlview,i,1,key,sizeof(key));
			if(key[0]!=0){
				char data[256]={0};
				ListView_GetItemText(hlview,i,2,data,sizeof(data));
				if(WritePrivateProfileString(sect,key,data,fname))
					result++;
			}
		}
	}
	return result;
}
int create_menu(HWND *hmenu)
{
	HMENU h;
	h=CreatePopupMenu();
	if(h!=0){
		InsertMenu(h,0xFFFFFFFF,MF_BYPOSITION|MF_STRING,IDC_TO_RIGHT,"COPY RIGHT -->");
		InsertMenu(h,0xFFFFFFFF,MF_BYPOSITION|MF_STRING,IDC_TO_LEFT,"COPY LEFT <--");
		InsertMenu(h,0xFFFFFFFF,MF_BYPOSITION|MF_SEPARATOR,0,0);
		InsertMenu(h,0xFFFFFFFF,MF_BYPOSITION|MF_STRING,IDC_OPEN_RIGHT,"OPEN RIGHT");
		InsertMenu(h,0xFFFFFFFF,MF_BYPOSITION|MF_STRING,IDC_OPEN_LEFT,"OPEN LEFT");
		InsertMenu(h,0xFFFFFFFF,MF_BYPOSITION|MF_SEPARATOR,0,0);
		InsertMenu(h,0xFFFFFFFF,MF_BYPOSITION|MF_STRING,IDC_SAVE_RIGHT,"SAVE RIGHT");
		InsertMenu(h,0xFFFFFFFF,MF_BYPOSITION|MF_STRING,IDC_SAVE_LEFT,"SAVE LEFT");
		InsertMenu(h,0xFFFFFFFF,MF_BYPOSITION|MF_SEPARATOR,0,0);
		InsertMenu(h,0xFFFFFFFF,MF_BYPOSITION|MF_STRING,CMD_CASE_TOGGLE,"data case sensitive");
		*hmenu=h;
	}
	return h!=0;
}
int load_icon(HWND hwnd)
{
	HICON hIcon = LoadIcon(ghinstance,MAKEINTRESOURCE(IDI_ICON1));
    if(hIcon){
		SendMessage(hwnd,WM_SETICON,ICON_SMALL,(LPARAM)hIcon);
		SendMessage(hwnd,WM_SETICON,ICON_BIG,(LPARAM)hIcon);
	}
	return 0;
}
int show_ttip(HWND hwnd,int ctrl,HWND *httip,char *text)
{
	int result=FALSE;
	HWND hctrl;
	hctrl=GetDlgItem(hwnd,ctrl);
	if(hctrl!=0){
		RECT rect={0};
		int x,y;
		GetWindowRect(hctrl,&rect);
		x=(rect.right+rect.left)/2;
		y=rect.top;
		destroy_tooltip(httip);
		result=create_tooltip(hwnd,httip,text,x,y);
		if(result){
			if(timer_event!=0)
				KillTimer(hwnd,timer_event);
			timer_event=SetTimer(hwnd,TIMER_ID,750,NULL);
		}
	}
	return result;
}
struct EDIT_PARAMS{
	char section[80];
	char key[80];
	char val[256];
};

LRESULT CALLBACK edit_dlg_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	static struct EDIT_PARAMS *ep=0;
	switch(msg){
	case WM_INITDIALOG:
		ep=lparam;
		if(ep!=0){
			SetDlgItemText(hwnd,IDC_SECTION,ep->section);
			SetDlgItemText(hwnd,IDC_KEY,ep->key);
			SetDlgItemText(hwnd,IDC_VALUE,ep->val);
		}
		SetFocus(GetDlgItem(hwnd,IDC_VALUE));
		break;
	case WM_COMMAND:
		switch(LOWORD(wparam)){
		case IDCANCEL:
			EndDialog(hwnd,FALSE);
			break;
		case IDOK:
			if(ep!=0)
				GetDlgItemText(hwnd,IDC_VALUE,ep->val,sizeof(ep->val));
			EndDialog(hwnd,ep!=0);
			break;
		}
	}
	return FALSE;
}
int edit_selection(HWND hlview,int idc)
{
	extern HINSTANCE ghinstance;
	int result=FALSE;
	int i,count;
	count=ListView_GetItemCount(hlview);
	for(i=0;i<count;i++){
		int state;
		state=ListView_GetItemState(hlview,i,LVIS_FOCUSED);
		if(state&(LVIS_FOCUSED|LVIS_SELECTED)){
			struct EDIT_PARAMS ep={0};
			ListView_GetItemText(hlview,i,0,ep.section,sizeof(ep.section));
			ListView_GetItemText(hlview,i,1,ep.key,sizeof(ep.key));
			ListView_GetItemText(hlview,i,2,ep.val,sizeof(ep.val));
			if(DialogBoxParam(ghinstance,MAKEINTRESOURCE(IDD_EDIT),hlview,edit_dlg_proc,&ep)){
				ListView_SetItemText(hlview,i,2,ep.val);
				result=TRUE;
			}
			break;
		}
	}
	if(result){
		if(idc=IDC_LVIEW_LEFT)
			left_dirty=TRUE;
		else
			right_dirty=TRUE;
		set_window_title(fname_left,left_dirty,fname_right,right_dirty);
	}
	return result;
}
int adjust_col_width(HWND hlview,int col,int dir)
{
	int width,result=TRUE;
	if(col<0){
		HWND hparent;
		RECT rect={0};
		int clamp,i;
		hparent=GetParent(hlview);
		GetWindowRect(hparent,&rect);
		clamp=rect.right-rect.left;
		clamp=(clamp/2)/3;
		for(i=0;i<3;i++)
			ListView_SetColumnWidth(hlview,i,clamp);
	}else{
		width=ListView_GetColumnWidth(hlview,col);
		if(dir>=0)
			width+=10;
		else
			width-=10;
		if(width>=0)
			ListView_SetColumnWidth(hlview,col,width);
	}
	return result;
}
LRESULT CALLBACK main_win_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	static int center=0,show_split=1;
	static HMENU hmenu=0;
#ifdef _DEBUG
//	if(msg==WM_CONTEXTMENU)
//	print_msg(msg,lparam,wparam,(int)hwnd);
#endif
	switch(msg){
	case WM_INITDIALOG:
		{
			HWND hleft,hright;
			setup_window(hwnd,ghinstance);
			ghwnd=hwnd;
			center=get_center(hwnd);
			resize_lview(hwnd,IDC_LVIEW_LEFT,IDC_LVIEW_RIGHT,center);
			move_buttons(hwnd,center);
			ghlvleft=hleft=GetDlgItem(hwnd,IDC_LVIEW_LEFT);
			ghlvright=hright=GetDlgItem(hwnd,IDC_LVIEW_RIGHT);
			init_listview(hleft);
			init_listview(hright);
			/*
			strncpy(fname_left,"C:\\DEV\\MSVC_Projects\\ini_compare\\gas.ini",sizeof(fname_left));
			strncpy(fname_right,"C:\\DEV\\MSVC_Projects\\ini_compare\\gas_Gilbarco.ini",sizeof(fname_left));
			populate_listview(hleft,hright,
				fname_left,
				fname_right,
				IsDlgButtonChecked(hwnd,IDC_CASE_SENSE));
			*/
			subclass_listview(hleft);
			subclass_listview(hright);
			create_menu(&hmenu);
			register_drag_drop(hwnd);
			restore_window_pos(hwnd);
			load_icon(hwnd);
			CheckDlgButton(hwnd,IDC_SHOW_SPLIT,show_split?BST_CHECKED:BST_UNCHECKED);
		}
		break;
	case WM_TIMER:
		if(timer_event!=0){
			if(KillTimer(hwnd,timer_event))
				timer_event=0;
		}
		destroy_tooltip(&ghttip);
		break;
	case WM_NOTIFY:
		{
			NMHDR *pnmh=lparam;
			if(pnmh!=0){
				switch(pnmh->idFrom){
				case IDC_LVIEW_LEFT:
				case IDC_LVIEW_RIGHT:
					switch(pnmh->code){
					case LVN_KEYDOWN:
					{
						LV_KEYDOWN *pnkd=lparam;
						int key;
						key=pnkd->wVKey;
						if(0x8000&GetKeyState(VK_CONTROL)){
							int idc;
							switch(key){
							case 'S':
								idc=pnmh->idFrom==IDC_LVIEW_LEFT?IDC_SAVE_LEFT:IDC_SAVE_RIGHT;
								SendMessage(hwnd,WM_COMMAND,MAKEWPARAM(idc,0),0);
								break;
							case 'O':
								idc=pnmh->idFrom==IDC_LVIEW_LEFT?IDC_OPEN_LEFT:IDC_OPEN_RIGHT;
								SendMessage(hwnd,WM_COMMAND,MAKEWPARAM(idc,0),0);
								break;
							case '0':
							case '1':
							case '2':
							case '3':
								{
									int dir=1;
									if(0x8000&GetKeyState(VK_SHIFT))
										dir=-1;
									adjust_col_width(pnmh->hwndFrom,key-'1',dir);

								}
								break;
							}
						}else{
							if(key==VK_F2){
								edit_selection(pnmh->hwndFrom,pnmh->idFrom);
							}
						}
					}
					break;
					}
				break;
				}
			}

		}
		break;
	case WM_SIZE:
		center=get_center(hwnd);
		resize_lview(hwnd,IDC_LVIEW_LEFT,IDC_LVIEW_RIGHT,center);
		move_buttons(hwnd,center);
		break;
	case WM_MOUSEWHEEL:
		{
			HWND hfocus=GetFocus();
			if(!(hfocus==ghlvleft || hfocus==ghlvright)){
				POINT pt;
				pt.x=LOWORD(lparam);
				pt.y=HIWORD(lparam);
				hfocus=WindowFromPoint(pt);
				if(hfocus==ghlvleft || hfocus==ghlvright)
					SendMessage(hfocus,WM_MOUSEWHEEL,wparam,lparam);
			}
		}
		break;
	case WM_SHOWWINDOW:
		{
			RECT rect={0};
			GetWindowRect(hwnd,&rect);
			move_console(rect.right,rect.top);
		}
		break;
	case WM_PAINT:
		break;
	case WM_DRAWITEM:
		{
			DRAWITEMSTRUCT *di=lparam;
			if(di!=0){
				if(di->CtlType==ODT_LISTVIEW){
					draw_item(di);
					return TRUE;
				}else if(di->CtlID==IDC_SPLIT){
					if(show_split)
						draw_split(di,hwnd);
					return TRUE;
				}
			}
		}
		break;
	case WM_APP:
		populate_listview(ghlvleft,ghlvright,fname_left,fname_right,IsDlgButtonChecked(hwnd,IDC_CASE_SENSE));
		return 0;
		break;
	case WM_APP+1:
		if(wparam==WM_LBUTTONDBLCLK){
			HWND hlview=lparam;
			if(hlview!=0){
				int idc;
				idc=GetDlgCtrlID(hlview);
				edit_selection(hlview,idc);
			}
		}
		break;
	case WM_CONTEXTMENU:
		{
			POINT pt;
			pt.x=LOWORD(lparam);
			pt.y=HIWORD(lparam);
			if(lparam==-1){
				RECT rect={0};
				GetWindowRect(hwnd,&rect);
				pt.x=(rect.right-rect.left)/2;
				pt.y=(rect.bottom-rect.top)/2;
			}
			TrackPopupMenu(hmenu,TPM_LEFTALIGN,pt.x,pt.y,0,hwnd,NULL);
		}
		break;
	case WM_COMMAND:
		switch(LOWORD(wparam)){
		case IDCANCEL:
			EndDialog(hwnd,0);
			break;
		case CMD_CASE_TOGGLE:
			{
				int set=!IsDlgButtonChecked(hwnd,IDC_CASE_SENSE);
				CheckDlgButton(hwnd,IDC_CASE_SENSE,set);
			}
		case IDC_CASE_SENSE:
			populate_listview(ghlvleft,ghlvright,fname_left,fname_right,IsDlgButtonChecked(hwnd,IDC_CASE_SENSE));
			break;
		case IDC_OPEN_LEFT:
			if(open_file(fname_left,sizeof(fname_left),TRUE))
				populate_listview(ghlvleft,ghlvright,fname_left,fname_right,IsDlgButtonChecked(hwnd,IDC_CASE_SENSE));
			break;
		case IDC_OPEN_RIGHT:
			if(open_file(fname_right,sizeof(fname_right),FALSE))
				populate_listview(ghlvleft,ghlvright,fname_left,fname_right,IsDlgButtonChecked(hwnd,IDC_CASE_SENSE));
			break;
		case IDC_TO_LEFT:
			move_data(ghlvright,ghlvleft);
			left_dirty=TRUE;
			set_window_title(fname_left,left_dirty,fname_right,right_dirty);
			break;
		case IDC_TO_RIGHT:
			move_data(ghlvleft,ghlvright);
			right_dirty=TRUE;
			set_window_title(fname_left,left_dirty,fname_right,right_dirty);
			break;
		case IDC_SAVE_LEFT:
			if(save_data(ghlvleft,fname_left)){
				show_ttip(hwnd,IDC_LVIEW_LEFT,&ghttip,"SAVED LEFT");
				left_dirty=FALSE;
				set_window_title(fname_left,left_dirty,fname_right,right_dirty);
			}
			break;
		case IDC_SAVE_RIGHT:
			if(save_data(ghlvright,fname_right)){
				show_ttip(hwnd,IDC_LVIEW_RIGHT,&ghttip,"SAVED RIGHT");
				right_dirty=FALSE;
				set_window_title(fname_left,left_dirty,fname_right,right_dirty);
			}
			break;
		case IDC_SHOW_SPLIT:
			{
				int chk=IsDlgButtonChecked(hwnd,IDC_SHOW_SPLIT);
				show_split=chk==BST_CHECKED?1:0;
				invalidate_split();
			}
			break;
		}
		break;
	case WM_CREATE:
		break;
	case WM_DESTROY:
		save_window_pos(hwnd);
		break;
	case WM_CLOSE:
		EndDialog(hwnd,0);
		break;
	}
	return 0;
}
int APIENTRY WinMain(HINSTANCE hinstance,
                     HINSTANCE hprevinstance,
                     LPSTR     cmd_line,
                     int       cmd_show)
{
    INITCOMMONCONTROLSEX ctrls={0};
	ghinstance=hinstance;
	ctrls.dwSize=sizeof(ctrls);
    ctrls.dwICC = ICC_LISTVIEW_CLASSES|ICC_TREEVIEW_CLASSES|ICC_BAR_CLASSES;
	InitCommonControlsEx(&ctrls);
	OleInitialize(0);
	init_ini_file();
#ifdef _DEBUG
	open_console();
#endif
	DialogBoxParam(hinstance,MAKEINTRESOURCE(IDD_DIALOG1),NULL,main_win_proc,0);
	return 0;
}



