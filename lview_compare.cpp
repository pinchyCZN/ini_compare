#if WINVER<0x500
	#define _WIN32_WINNT 0x500
#endif
#pragma warning(disable:4786)
#include <windows.h>
#include <Commctrl.h>
#include <stdio.h>
#include <list>
#include <algorithm>
#include <vector>
#include <string>
using namespace std;

extern "C" {
int populate_listview(HWND hleft,HWND hright,char *fleft,char *fright,int);
int lv_insert_data(HWND hlistview,int row,int col,const char *str);
int get_str_width(HWND hwnd,const char *str);
int lv_get_col_text(HWND hlistview,int index,char *str,int size);
int set_window_title(const char *f1,const char *f2);
int invalidate_split();
}

bool compare_nocase(const string &a,const string &b)
{
	unsigned int i=0;
	while ( (i<a.length()) && (i<b.length()) )
	{
		if (tolower(a[i])<tolower(b[i])) return true;
		else if (tolower(a[i])>tolower(b[i])) return false;
		++i;
	}
	return ( a.length() < b.length() );
	return false;
}

class str_list
{
public:
	vector<string> list;
	void add(const char *s)
	{
		list.push_back(s);
	}
	int count()
	{
		return list.size();
	}
	int clear()
	{
		list.clear();
		return 0;
	}
	int remove(int index)
	{
		int result=FALSE;
		if(index>=0 && index<list.size()){
			std::vector<string>::iterator it=list.begin();
			advance(it,index);
			list.erase(it);
			result=TRUE;
		}
		return result;
	}
	const char *strings(int index)
	{
		const char *result=0;
		if(index>=0 && index<list.size()){
			string s=list[index];
			result=s.c_str();
		}
		return result;
	}
	int sort()
	{
		std::sort(list.begin(),list.end(),compare_nocase);
		return 0;
	}
};
struct KEY{
	string key;
	string val;
	int meta;
};
struct SECTION{
	string name;
	vector<KEY> keys;
	int meta;
};
bool compare_section(const SECTION &a,const SECTION &b)
{
	return compare_nocase(a.name,b.name);
}
bool compare_key(const KEY &a,const KEY &b)
{
	return compare_nocase(a.key,b.key);
}
class ini_file
{
public:
	string fname;
	vector<SECTION> sections;
	int ini_file::load_ini(char *_fname);
	void add_section(const char *name)
	{
		SECTION s;
		s.name=name;
		sections.push_back(s);
	}
	void sort_sections()
	{
		sort(sections.begin(),sections.end(),compare_section);
	}
	void sort_keys()
	{
		int i;
		for(i=0;i<sections.size();i++){
			sort(sections[i].keys.begin(),sections[i].keys.end(),compare_key);
		}
	}
	SECTION * find_section(const char *name)
	{
		int i;
		SECTION *result=0;
		for(i=0;i<sections.size();i++){
			if(stricmp(sections[i].name.c_str(),name)==0){
				result=&sections[i];
				break;
			}
		}
		return result;
	}
	KEY * find_key(SECTION &section,const char *name){
		int i;
		KEY *result=0;
		for(i=0;i<section.keys.size();i++){
			if(stricmp(section.keys[i].key.c_str(),name)==0){
				result=&section.keys[i];
				break;
			}
		}
		return result;
	}
	void clear()
	{
		sections.clear();
	}

};
int add_key_val(char *in,string &key,string &val)
{
	string tmp=in;
	string::size_type pos;
	pos=tmp.find("=");
	if(pos!=(std::string::npos)){
		key=tmp.substr(0,pos);
		val=tmp.substr(pos+1);
	}else{
		key=in;
		val="";
	}
	return 0;
}
int ini_file::load_ini(char *_fname)
{
	int result=FALSE;
	char *tmp;
	int size=0x100000;
	sections.clear();
	if(_fname==0)
		return result;
	fname=_fname;
	tmp=(char*)calloc(1,size);
	if(tmp!=0){
		int i,len=0;
		GetPrivateProfileSectionNames(tmp,size,fname.c_str());
		for(i=0;i<size;i++){
			if(tmp[i]==0){
				if(len==0)
					break;
				add_section(tmp+i-len);
				len=0;
			}else
				len++;
		}
		for(i=0;i<sections.size();i++){
			int j;
			memset(tmp,0,size);
			len=0;
			GetPrivateProfileSection(sections[i].name.c_str(),tmp,size,fname.c_str());
			for(j=0;j<size;j++){
				if(tmp[j]==0){
					KEY k;
					if(len==0)
						break;
					add_key_val(tmp+j-len,k.key,k.val);
					sections[i].keys.push_back(k);
					len=0;
				}else
					len++;
			}
			result=TRUE;
		}
		sort_sections();
		sort_keys();
	}
	return result;
}
ini_file ini_left;
ini_file ini_right;

int copy_ini_meta(ini_file &center,ini_file &in,int meta)
{
	int i;
	for(i=0;i<in.sections.size();i++){
		SECTION *s=0;
		s=center.find_section(in.sections[i].name.c_str());
		if(s!=0){
			int j;
			for(j=0;j<in.sections[i].keys.size();j++){
				KEY k;
				k.key=in.sections[i].keys[j].key;
				k.val=in.sections[i].keys[j].val;
				k.meta=meta;
				s->keys.push_back(k);
			}
		}else{
			int j;
			SECTION sect;
			sect.name=in.sections[i].name;
			sect.meta=meta;
			for(j=0;j<in.sections[i].keys.size();j++){
				KEY key;
				key.key=in.sections[i].keys[j].key;
				key.val=in.sections[i].keys[j].val;
				key.meta=meta;
				sect.keys.push_back(key);
			}
			center.sections.push_back(sect);
		}
	}
	return 0;
}
bool compare_section_meta(const SECTION &a,const SECTION &b)
{
	bool result=false;
	if(stricmp(a.name.c_str(),b.name.c_str())==0){
		result=a.meta<b.meta;
	}else
		result=compare_nocase(a.name,b.name);
	return result;
}
bool compare_key_meta(const KEY &a,const KEY &b)
{
	bool result=false;
	if(stricmp(a.key.c_str(),b.key.c_str())==0){
		result=a.meta<b.meta;
	}
	else
		result=compare_nocase(a.key,b.key);
	return result;
}

int get_col_widths(HWND hlview,int *w,int count)
{
	int i;
	for(i=0;i<count;i++){
		char tmp[80]={0};
		lv_get_col_text(hlview,i,tmp,sizeof(tmp));
		w[i]=get_str_width(hlview,tmp);
	}
	return count;
}
int set_item_params(HWND hleft,HWND hright,int case_sense)
{
	int i,count;
	count=ListView_GetItemCount(hleft);
	for(i=0;i<count;i++){
		char a[80]={0},b[80]={0};
		LV_ITEM lvi={0};
		lvi.mask=LVIF_PARAM;
		lvi.iItem=i;
		ListView_GetItemText(hleft,i,1,a,sizeof(a));
		ListView_GetItemText(hright,i,1,b,sizeof(b));
		if(stricmp(a,b)!=0){
			lvi.lParam=1;
			ListView_SetItem(hleft,&lvi);
			ListView_SetItem(hright,&lvi);
		}else{
			int (*scmp)(const char *a,const char *b)=0;
			ListView_GetItemText(hleft,i,2,a,sizeof(a));
			ListView_GetItemText(hright,i,2,b,sizeof(b));
			if(case_sense)
				scmp=strcmp;
			else
				scmp=stricmp;
			if(scmp(a,b)!=0)
				lvi.lParam=1;
			else
				lvi.lParam=0;
			ListView_SetItem(hleft,&lvi);
			ListView_SetItem(hright,&lvi);
		}
	}
	return 0;
}
int clear_empty_sections(HWND hlview)
{
	int i,count,result=0;
	count=ListView_GetItemCount(hlview);
	for(i=0;i<count;i++){
		char tmp[80]={0};
		ListView_GetItemText(hlview,i,1,tmp,sizeof(tmp));
		if(tmp[0]==0){
			ListView_SetItemText(hlview,i,0,"");
			result++;
		}
	}
	return result;
}
int get_top_ypos(HWND hlview,int *ypos)
{
	RECT rtop={0},rect={0};
	int index;
	index=ListView_GetTopIndex(hlview);
	ListView_GetItemRect(hlview,0,&rtop,LVIR_BOUNDS);
	ListView_GetItemRect(hlview,index,&rect,LVIR_BOUNDS);
	*ypos=rect.top-rtop.top;
	return TRUE;
}
int populate_listview(HWND hleft,HWND hright,char *fleft,char *fright,int case_sense)
{
	int result=FALSE;
	int i,row=0;
	int top_ypos=0;
	int max_width=0;
	ini_file center;
	ini_left.load_ini(fleft);
	ini_right.load_ini(fright);
	int widths[2][3];
	FILE *f=0;
//	f=fopen("c:\\temp\\list.txt","wb");
	get_col_widths(hleft,widths[0],sizeof(widths[0])/sizeof(widths[0][0]));
	get_col_widths(hright,widths[1],sizeof(widths[1])/sizeof(widths[1][0]));

	copy_ini_meta(center,ini_left,0);
	copy_ini_meta(center,ini_right,1);
	sort(center.sections.begin(),center.sections.end(),compare_section_meta);
	for(i=0;i<center.sections.size();i++){
		sort(center.sections[i].keys.begin(),center.sections[i].keys.end(),compare_key_meta);
	}
	get_top_ypos(hleft,&top_ypos);
	ListView_DeleteAllItems(hleft);
	ListView_DeleteAllItems(hright);
	{
		RECT rect={0};
		GetWindowRect(hleft,&rect);
		max_width=rect.right-rect.left;
	}
	for(i=0;i<center.sections.size();i++){
		int j,w;
		string last_key="";
		int last_meta=0;
		int key_count;
//		printf("%s\n",center.sections[i].name.c_str());
		w=get_str_width(hleft,center.sections[i].name.c_str());
		if(w>widths[0][0]){
			widths[0][0]=widths[1][0]=w;
		}
		if(f!=0)
			fprintf(f,"%s\n",center.sections[i].name.c_str());
		if(i!=0)
			row++;
		key_count=center.sections[i].keys.size();
		if(0==key_count){
			const char *name=center.sections[i].name.c_str();
			lv_insert_data(hleft,row,0,name);
			lv_insert_data(hright,row,0,name);
		}
		else{
			for(j=0;j<key_count;j++){
				string tmp;
				int side=0;
				HWND hlview;
				string key,val,sname;
				sname=center.sections[i].name;
				key=center.sections[i].keys[j].key;
				val=center.sections[i].keys[j].val;
				side=center.sections[i].keys[j].meta;
				if(stricmp(last_key.c_str(),key.c_str())!=0){
					if(j!=0)
						row++;
					lv_insert_data(hleft,row,0,sname.c_str());
					lv_insert_data(hright,row,0,sname.c_str());
					w=get_str_width(hleft,key.c_str());
					if(w>widths[0][1]){
						if(max_width>20 && w>max_width)
							w=max_width*.8;
						widths[0][1]=widths[1][1]=w;
					}
				}
				if(0==side)
					hlview=hleft;
				else
					hlview=hright;
	//			printf("  %02i %s=%s\n",side,key.c_str(),val.c_str());
				if(f!=0)
					fprintf(f,"  %02i %s=%s (%i)\n",side,key.c_str(),val.c_str(),row);
				lv_insert_data(hlview,row,1,key.c_str());
				lv_insert_data(hlview,row,2,val.c_str());
				w=get_str_width(hlview,val.c_str());
				if(w>widths[side][2]){
					if(max_width>20 && w>max_width)
						w=max_width*.8;
					widths[side][2]=w;
				}

				last_key=key;
				last_meta=side;
			}
		}
	}
	if(f!=0)
		fclose(f);
	clear_empty_sections(hleft);
	clear_empty_sections(hright);
	set_item_params(hleft,hright,case_sense);
	ListView_Scroll(hleft,0,top_ypos);
	ListView_Scroll(hright,0,top_ypos);
	
	for(i=0;i<sizeof(widths[0])/sizeof(widths[0][0]);i++){
		ListView_SetColumnWidth(hleft,i,widths[0][i]+7);
		ListView_SetColumnWidth(hright,i,widths[1][i]+7);
	}
	set_window_title(ini_left.fname.c_str(),ini_right.fname.c_str());
	invalidate_split();
	return result;
}