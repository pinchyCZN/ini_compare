#if WINVER<0x500
	#define WINVER 0x500
	#define _WIN32_WINNT 0x500
#endif
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <shlwapi.h>
#include <Shlobj.h>
#include "resource.h"

#define APP_NAME "INI_COMPARE"
char ini_file[MAX_PATH]={0};
int is_path_directory_wc(WCHAR *path)
{
	int attrib;
	attrib=GetFileAttributesW(path);
	if((attrib!=0xFFFFFFFF) && (attrib&FILE_ATTRIBUTE_DIRECTORY))
		return TRUE;
	else
		return FALSE;
}
int is_path_directory(char *path)
{
	int attrib;
	attrib=GetFileAttributes(path);
	if((attrib!=0xFFFFFFFF) && (attrib&FILE_ATTRIBUTE_DIRECTORY))
		return TRUE;
	else
		return FALSE;
}
int get_ini_value(char *section,char *key,int *val)
{
	char str[255]={0};
	int result=0;
	if(ini_file[0]!=0){
		result=GetPrivateProfileString(section,key,"",str,sizeof(str),ini_file);
		if(str[0]!=0)
			*val=atoi(str);
	}
	return result>0;
}
int get_ini_str(char *section,char *key,char *str,int size)
{
	int result=0;
	char tmpstr[1024]={0};
	if(ini_file[0]!=0){
		result=GetPrivateProfileString(section,key,"",tmpstr,sizeof(tmpstr),ini_file);
		if(result>0)
			strncpy(str,tmpstr,size);
	}
	return result>0;
}
int write_ini_value(char *section,char *key,int val)
{
	int result=FALSE;
	char str[20]={0};
	_snprintf(str,sizeof(str),"%i",val);
	if(ini_file[0]!=0){
		if(WritePrivateProfileString(section,key,str,ini_file)!=0)
			result=TRUE;
		else
			result=FALSE;
	}
	return result;
}
int write_ini_str(char *section,char *key,char *str)
{
	int result=FALSE;
	if(ini_file[0]!=0){
		if(WritePrivateProfileString(section,key,str,ini_file)!=0)
			result=TRUE;
		else
			result=FALSE;
	}
	return result;
}
int get_appdata_folder(char *path,int size)
{
	int found=FALSE;
	ITEMIDLIST *pidl;
	IMalloc	*palloc;
	HWND hwindow=0;
	if(path==0 || size<MAX_PATH)
		return found;
	if(SHGetSpecialFolderLocation(hwindow,CSIDL_APPDATA,&pidl)==NOERROR){
		if(SHGetPathFromIDList(pidl,path)){
			_snprintf(path,size,"%s\\%s",path,APP_NAME);
			found=TRUE;
		}
		if(SHGetMalloc(&palloc)==NOERROR){
			palloc->lpVtbl->Free(palloc,pidl);
			palloc->lpVtbl->Release(palloc);
		}
	}
	return found;
}
int init_ini_file()
{
	char path[MAX_PATH];
	memset(ini_file,0,sizeof(ini_file));
	path[0]=0;
	get_appdata_folder(path,sizeof(path));
	if(!is_path_directory(path))
		CreateDirectory(path,NULL);
	_snprintf(ini_file,sizeof(ini_file)-1,"%s\\" APP_NAME ".ini",path);
	ini_file[sizeof(ini_file)-1]=0;
	if(path[0]==0)
		ini_file[0]=0;
	return ini_file[0]!=0;
}

int clamp_window(RECT *rwin)
{
	HMONITOR hmon;
	hmon=MonitorFromRect(rwin,MONITOR_DEFAULTTONEAREST);
	if(hmon){
		MONITORINFO mi;
		mi.cbSize=sizeof(mi);
		if(GetMonitorInfo(hmon,&mi)){
			int x,y,cx,cy;
			RECT rmon;
			rmon=mi.rcWork;
			x=rwin->left;
			y=rwin->top;
			cx=rwin->right-rwin->left;
			cy=rwin->bottom-rwin->top;
			if(x<rmon.left)
				x=rmon.left;
			if(y<rmon.top)
				y=rmon.top;
			if(cx>(rmon.right-rmon.left))
				cx=rmon.right-rmon.left;
			if(cy>(rmon.bottom-rmon.top))
				cy=rmon.bottom-rmon.top;
			if((x+cx)>rmon.right)
				x=rmon.right-cx;
			if((y+cy)>rmon.bottom)
				y=rmon.bottom-cy;
			rwin->left=x;
			rwin->top=y;
			rwin->right=x+cx;
			rwin->bottom=y+cy;
		}
	}
	return 0;
}
int restore_window_pos(HWND hwnd)
{
	const char *SECT="WINDOW";
	int x=0,y=0,w=0,h=0,maximized=0;
	if(get_ini_value(SECT,"XPOS",&x) && get_ini_value(SECT,"YPOS",&y)
		&& get_ini_value(SECT,"WIDTH",&w) && get_ini_value(SECT,"HEIGHT",&h)){
		WINDOWPLACEMENT wp={0};
		int show=SW_SHOWNORMAL;
		get_ini_value(SECT,"MAXIMIZED",&maximized);
		if(maximized)
			show=SW_SHOWMAXIMIZED;
		wp.length=sizeof(wp);
		wp.showCmd=show;
		wp.rcNormalPosition.left=x;
		wp.rcNormalPosition.top=y;
		wp.rcNormalPosition.right=x+w;
		wp.rcNormalPosition.bottom=y+h;
		clamp_window(&wp.rcNormalPosition);
		if(w>50 && h>50)
			SetWindowPlacement(hwnd,&wp);
	}
	return 0;
}
int save_window_pos(HWND hwnd)
{
	const char *SECT="WINDOW";
	WINDOWPLACEMENT wp={0};
	int val;
	wp.length=sizeof(wp);
	GetWindowPlacement(hwnd,&wp);
	write_ini_value(SECT,"XPOS",wp.rcNormalPosition.left);
	write_ini_value(SECT,"YPOS",wp.rcNormalPosition.top);
	write_ini_value(SECT,"WIDTH",wp.rcNormalPosition.right-wp.rcNormalPosition.left);
	write_ini_value(SECT,"HEIGHT",wp.rcNormalPosition.bottom-wp.rcNormalPosition.top);
	val=0!=(wp.flags&WPF_RESTORETOMAXIMIZED);
	write_ini_value(SECT,"MAXIMIZED",val);
	return 0;
}