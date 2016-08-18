#include <windows.h>
#include <commctrl.h>
#include <oleidl.h>

static 	IDropTarget droptarget;
static HWND ghwnd=0;
static HWND ghdrop_ttip=0;

int create_tooltip(HWND hwnd,HWND *htt_out,char *tt_text,int x, int y)
{
	int result=FALSE;
	if(htt_out==0)
		return result;
	if(tt_text[0]!=0){
		HWND hwndTT;
		hwndTT=CreateWindowEx(WS_EX_TOPMOST,
			TOOLTIPS_CLASS,NULL,
			WS_POPUP|TTS_NOPREFIX|TTS_ALWAYSTIP,        
			CW_USEDEFAULT,CW_USEDEFAULT,
			CW_USEDEFAULT,CW_USEDEFAULT,
			hwnd,NULL,NULL,NULL);
		if(hwndTT!=0){
			TOOLINFO ti;
			ti.cbSize = sizeof(TOOLINFO);
			ti.uFlags = TTF_IDISHWND|TTF_TRACK|TTF_ABSOLUTE;
			ti.hwnd = hwndTT;
			ti.uId = hwndTT;
			ti.lpszText = tt_text;
			SendMessage(hwndTT,TTM_ADDTOOL,0,&ti);
			SendMessage(hwndTT,TTM_UPDATETIPTEXTA,0,&ti);
			SendMessage(hwndTT,TTM_SETMAXTIPWIDTH,0,640); //makes multiline tooltips
			SendMessage(hwndTT,TTM_TRACKPOSITION,0,MAKELONG(x,y)); 
			SendMessage(hwndTT,TTM_TRACKACTIVATE,TRUE,&ti);
			*htt_out=hwndTT;
			result=TRUE;
		}
	}
	return result;
}
int update_tooltip_text(HWND hwndTT,char *tt_text)
{
	int result=0;
	if(hwndTT!=0){
		TOOLINFO ti;
		ti.cbSize = sizeof(TOOLINFO);
		ti.uFlags = TTF_IDISHWND|TTF_TRACK|TTF_ABSOLUTE;
		ti.hwnd = hwndTT;
		ti.uId = hwndTT;
		ti.lpszText = tt_text;
		result=SendMessage(hwndTT,TTM_UPDATETIPTEXTA,0,&ti);
	}
	return result;
}
int destroy_tooltip(HWND *hwndTT)
{
	int result=0;
	HWND htmp;
	if(hwndTT==0)
		return result;
	htmp=*hwndTT;
	if(htmp!=0){
		result=DestroyWindow(htmp);
		*hwndTT=0;
	}
	return result;
}
int get_correct_msg(char *msg,int len,int left)
{
	int shift=FALSE,ctrl=FALSE;
	static char *cmsg="CTRL=FORCE LEFT DROP";
	static char *smsg="SHIFT=FORCE RIGHT DROP";
	char *sptr;

	if(msg==0 || len<=0)
		return FALSE;
	if(left)
		sptr="LEFT DROP";
	else
		sptr="RIGHT DROP";
	shift=GetKeyState(VK_SHIFT)&0x8000;
	ctrl=GetKeyState(VK_CONTROL)&0x8000;
	if(ctrl==FALSE && shift==FALSE)
		_snprintf(msg,len,"%s\r\n\r\n%s\r\n%s",sptr,cmsg,smsg);
	else if(ctrl && shift==FALSE)
		_snprintf(msg,len,"%s",cmsg);
	else if(shift && ctrl==FALSE)
		_snprintf(msg,len,"%s",smsg);
	else
		_snprintf(msg,len,"%s",sptr);
	msg[len-1]=0;
	return TRUE;
}

//	IUnknown::AddRef
static ULONG STDMETHODCALLTYPE idroptarget_addref (IDropTarget* This)
{
  return S_OK;
}

//	IUnknown::QueryInterface
static HRESULT STDMETHODCALLTYPE
idroptarget_queryinterface (IDropTarget *This,
			       REFIID          riid,
			       LPVOID         *ppvObject)
{
  return S_OK;
}


//	IUnknown::Release
static ULONG STDMETHODCALLTYPE
idroptarget_release (IDropTarget* This)
{
  return S_OK;
}

int is_left(POINTL pt)
{
	RECT rect={0};
	int center;
	ScreenToClient(ghwnd,&pt);
	GetWindowRect(ghwnd,&rect);
	center=(rect.right-rect.left)/2;
	return pt.x<center;
}
//	IDropTarget::DragEnter
static HRESULT STDMETHODCALLTYPE idroptarget_dragenter(IDropTarget* This, IDataObject * pDataObject, DWORD grfKeyState, POINTL pt, DWORD * pdwEffect)
{
	if(ghwnd!=0){
		RECT rect={0};
		char msg[100]={0};
		int left;
		left=is_left(pt);
		get_correct_msg(msg,sizeof(msg),left);
		GetWindowRect(ghwnd,&rect);
		create_tooltip(ghwnd,&ghdrop_ttip,msg,rect.left,rect.top);
	}
	return S_OK;
}

//	IDropTarget::DragOver
static HRESULT STDMETHODCALLTYPE idroptarget_dragover(IDropTarget* This, DWORD grfKeyState, POINTL pt, DWORD * pdwEffect)
{
	char msg[100]={0};
	int left;
	left=is_left(pt);
	get_correct_msg(msg,sizeof(msg),left);
	update_tooltip_text(ghdrop_ttip,msg);
	return S_OK;
}

//	IDropTarget::DragLeave
static HRESULT STDMETHODCALLTYPE idroptarget_dragleave(IDropTarget* This)
{
	destroy_tooltip(&ghdrop_ttip);
	return S_OK;
}

//	IDropTarget::Drop
static HRESULT STDMETHODCALLTYPE idroptarget_drop(IDropTarget* This, IDataObject * pDataObject, DWORD grfKeyState, POINTL pt, DWORD * pdwEffect)
{
	FORMATETC fmtetc={CF_HDROP,NULL,DVASPECT_CONTENT,-1,TYMED_HGLOBAL};
	STGMEDIUM stg;
	//printf("drag drop\n");
	if(S_OK==pDataObject->lpVtbl->GetData(pDataObject,&fmtetc,&stg)){
		int left;
		left=is_left(pt);
			process_drop(ghwnd,(HANDLE)stg.hGlobal,GetKeyState(VK_CONTROL)&0x8000,
				GetKeyState(VK_SHIFT)&0x8000,left);

	}
	destroy_tooltip(&ghdrop_ttip);
	return S_OK;
}
static IDropTargetVtbl idt_vtbl={
	idroptarget_queryinterface,
	idroptarget_addref,
	idroptarget_release,
	idroptarget_dragenter,
	idroptarget_dragover,
	idroptarget_dragleave,
	idroptarget_drop
};
int register_drag_drop(HWND hwnd)
{
	ghwnd=hwnd;
	droptarget.lpVtbl=(IDropTargetVtbl*)&idt_vtbl;
	return RegisterDragDrop(hwnd,&droptarget);
}

int process_drop(HWND hwnd,HANDLE hdrop,int ctrl,int shift,int left)
{
	int i,count,dropped;
	char str[MAX_PATH];
	count=DragQueryFile(hdrop,-1,NULL,0);
	if(ctrl && shift==FALSE)
		left=TRUE;
	else if(shift && ctrl==FALSE)
		left=FALSE;
	dropped=0;
	for(i=0;i<count;i++){
		str[0]=0;
		DragQueryFile(hdrop,i,str,sizeof(str));
		if(!is_path_directory(str)){
			if(left)
				update_left_fname(str);
			else
				update_right_fname(str);
			left=!left;
			dropped++;
			if(dropped>=2)
				break;
		}
	}
	DragFinish(hdrop);
	PostMessage(hwnd,WM_APP,0,0);
	return 0;
}
