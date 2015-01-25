// WindowTest.cpp : 定义应用程序的入口点。
//

#include "stdafx.h"
#include "WindowTest.h"
#include"atlstr.h"
#include<shellapi.h>

#pragma comment(lib,"Lualib.lib")
extern "C"{
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}
#define MAX_LOADSTRING 100

// 全局变量:
HINSTANCE hInst;								// 当前实例
TCHAR szTitle[MAX_LOADSTRING];					// 标题栏文本
TCHAR szWindowClass[MAX_LOADSTRING];			// 主窗口类名

HDC dc;
HBRUSH br;
HBRUSH br2;
HBRUSH br3;
HHOOK MouseHook=NULL;
HINSTANCE hnc;

HWND g_hWnd = NULL;

lua_State *LuaStack;
CString ErrorLog=L"";

// 此代码模块中包含的函数的前向声明:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

#define BIT0 0x01
#define BIT1 0x02
#define BIT2 0x04
#define BIT3 0x08
#define BIT4 0x10
#define BIT5 0x20
#define BIT6 0x40
#define BIT7 0x80

#define StickUse BIT0
#define StickEnb BIT1
#define StickK2M BIT2
#define StickK1  BIT4
#define StickK2  BIT5
#define StickK3  BIT6
#define StickK4  BIT7

#define StickSetFlag(x,b) (x|=b)
#define StickClrFlag(x,b) (x&=~b)

#define ButtonUse BIT0
#define ButtonEnb BIT1
#define ButtonCst BIT2
#define ButtonK0  BIT3
#define ButtonK2M BIT4

#define ButtonSetFlag(x,b) (x|=b)
#define ButtonClrFlag(x,b) (x&=~b)

typedef struct
{
	RECT rt;
	RECT srt;
	RECT mrt;
	POINT Pos;
	POINT Bise;
	POINT NPOS;
	char code[4];
	int sid;
	unsigned char BitFlag;
}Stick;

typedef struct
{
	RECT rt;
	POINT Bise;
	int KeyVal;
	WCHAR *str;
	int color;
	int sid;
	unsigned char BitFlag;
}Button;

#define StickMax 10
Stick *StickList[StickMax]={NULL};
int StickCount=0;

#define ButtonMax 10
Button *ButtonList[ButtonMax] = { NULL };
int ButtonCount = 0;



#define MAXPOINT 10
TOUCHINPUT TouchBuff[MAXPOINT];
TOUCHINPUT *TouchBuffPrt=TouchBuff;
int TouchPointCount=0;

bool CheckBox(POINT p,RECT r)
{
	if((p.x>r.left)&&(p.x<r.right)&&(p.y>r.top)&&(p.y<r.bottom))
		return true;
	return false;
}
#define KEY_DOWN	0
#define KEY_UP		1

int UNICODE2ANSI(CString cs, char *buff)
{
	int count = cs.GetLength();
	WCHAR* str = (WCHAR*)(LPCTSTR)cs;
	return WideCharToMultiByte(CP_ACP, 0, str, -1, buff, 100, NULL, NULL);
}

CString ANSI2UNICODE(const char* s)
{
	CString out;
	int count = ::MultiByteToWideChar(CP_ACP, 0, s, -1, NULL, 0);
	WCHAR *pbuff = new WCHAR[count];
	::MultiByteToWideChar(CP_ACP, 0, s, -1, pbuff, count);
	out.Format(_T("%s"), pbuff);
	delete[] pbuff;
	return out;
}

void KeyEvent(unsigned short key,int flag)
{
	KEYBDINPUT kD;
	KEYBDINPUT kU;
	INPUT keyval[2];
	kD.wVk=kU.wVk=key;
	kD.wScan=::MapVirtualKey(key,0);
	kU.wScan=::MapVirtualKey(key,0)+0x80;//VK_LCONTROL
	kD.time=kU.time=0;
	kD.dwExtraInfo=kU.dwExtraInfo=(WORD)::GetMessageExtraInfo();
	kD.dwFlags=kU.dwFlags=KEYEVENTF_SCANCODE;
	kU.dwFlags=KEYEVENTF_KEYUP;
	keyval[0].type=keyval[1].type=INPUT_KEYBOARD;
	keyval[0].ki=kD;
	keyval[1].ki=kU;
	switch(flag)
	{
		case 0: ::SendInput(1,keyval,sizeof(INPUT)); break;
		case 1: ::SendInput(1,&keyval[1],sizeof(INPUT)); break;
		case 2: ::SendInput(2,keyval,sizeof(INPUT)); break;
	}
}

void MouseEvent(POINT p, int flag)
{
	INPUT keyval[1];
	keyval[0].mi.dx = p.x;
	keyval[0].mi.dy = p.y;
	keyval[0].mi.dwExtraInfo = 0;
	keyval[0].mi.time = 0;
	keyval[0].mi.mouseData = 0;
	keyval[0].mi.dwFlags = flag;
	keyval[0].type = INPUT_MOUSE;
	::SendInput(1, keyval, sizeof(INPUT));
}

void MouseEventEx(int flag, int data)
{
	POINT p = { 0, 0 };
	INPUT keyval[1];
	keyval[0].mi.dx = p.x;
	keyval[0].mi.dy = p.y;
	keyval[0].mi.dwExtraInfo = 0;
	keyval[0].mi.time = 0;
	keyval[0].mi.mouseData = data;
	keyval[0].mi.dwFlags = flag;
	keyval[0].type = INPUT_MOUSE;
	::SendInput(1, keyval, sizeof(INPUT));
}

void CheckStickDown(POINT p,int id)
{
	for(int i=0;i<StickCount;i++)
	{
		if(!CheckBox(p,StickList[i]->rt))
			continue;
		if(!(StickList[i]->BitFlag&StickEnb))
			continue;
		if(!(StickList[i]->BitFlag&StickUse))
		{
			StickList[i]->BitFlag|=StickUse;
			StickList[i]->sid=id;
			return;
		}
	}
	for (int i = 0; i < ButtonCount; i++)
	{
		if (!CheckBox(p, ButtonList[i]->rt))
			continue;
		if (!(ButtonList[i]->BitFlag&ButtonEnb))
			continue;
		if (!(ButtonList[i]->BitFlag&ButtonUse))
		{
			ButtonSetFlag(ButtonList[i]->BitFlag, ButtonUse);
			ButtonList[i]->sid = id;
			FillRect(dc, &ButtonList[i]->rt, br2);
			SetBkColor(dc, RGB(208, 208, 208));
			CString c;
			c.Format(L"%c", ButtonList[i]->KeyVal);
			TextOut(dc, ButtonList[i]->Bise.x, ButtonList[i]->Bise.y, c, c.GetLength());
			if (!(ButtonList[i]->BitFlag&ButtonK0)){
				ButtonSetFlag(ButtonList[i]->BitFlag, ButtonK0);
				if (ButtonList[i]->BitFlag&ButtonK2M)
				{
					POINT p = { 0, 0 };
					if (ButtonList[i]->KeyVal)
						MouseEvent(p, MOUSEEVENTF_LEFTDOWN);
					else
						MouseEvent(p, MOUSEEVENTF_RIGHTDOWN);
				}
				else
				{
					KeyEvent(ButtonList[i]->KeyVal, KEY_DOWN);
				}
				lua_getglobal(LuaStack, "ButtonDownCall");
				lua_pushinteger(LuaStack, i);
				if (lua_pcall(LuaStack, 1, 0, 0))
				{
					ErrorLog += ANSI2UNICODE(lua_tostring(LuaStack, -1));
					ErrorLog += _T("\r\n");
				}
			}
			return;
		}
	}
}
void StickDesKey(Stick *sk)
{
	for (int i = 0; i < 4; i++){
		if (sk->BitFlag&(0x10 << i)){
			KeyEvent(sk->code[i], KEY_UP);
		}
	}
	sk->BitFlag &= 0x0f;
}
void CheckStickUp(POINT p,int id)
{
	RECT r;
	for(int i=0;i<StickCount;i++)
	{
		if(StickList[i]->sid!=id)
			continue;
		if(!(StickList[i]->BitFlag&StickEnb))
			continue;
		if((StickList[i]->BitFlag&StickUse))
		{
			StickList[i]->BitFlag&=~StickUse;
			FillRect(dc,&StickList[i]->rt,br);
			FillRect(dc,&StickList[i]->mrt,br3);
			r.left=StickList[i]->srt.left+StickList[i]->Pos.x;
			r.top=StickList[i]->srt.top+StickList[i]->Pos.y;
			r.right=StickList[i]->srt.right+StickList[i]->Pos.x;
			r.bottom=StickList[i]->srt.bottom+StickList[i]->Pos.y;
			FillRect(dc,&r,br2);
			StickDesKey(StickList[i]);
			return;
		}
	}
	for (int i = 0; i < ButtonCount; i++)
	{
		if (ButtonList[i]->sid != id)
			continue;
		if (!(ButtonList[i]->BitFlag&ButtonEnb))
			continue;
		if ((ButtonList[i]->BitFlag&ButtonUse))
		{
			ButtonClrFlag(ButtonList[i]->BitFlag, ButtonUse);
			FillRect(dc, &ButtonList[i]->rt, br);
			SetBkColor(dc, RGB(239, 239, 239));
			CString c;
			c.Format(L"%c", ButtonList[i]->KeyVal);
			TextOut(dc, ButtonList[i]->Bise.x, ButtonList[i]->Bise.y, c, c.GetLength());
			if ((ButtonList[i]->BitFlag&ButtonK0)){
				ButtonClrFlag(ButtonList[i]->BitFlag, ButtonK0);
				if (ButtonList[i]->BitFlag&ButtonK2M)
				{
					POINT p = { 0, 0 };
					if (ButtonList[i]->KeyVal)
						MouseEvent(p, MOUSEEVENTF_LEFTUP);
					else
						MouseEvent(p, MOUSEEVENTF_RIGHTUP);
				}
				else
				{
					KeyEvent(ButtonList[i]->KeyVal, KEY_UP);
				}
				lua_getglobal(LuaStack, "ButtonUpCall");
				lua_pushinteger(LuaStack, i);
				if (lua_pcall(LuaStack, 1, 0, 0))
				{
					ErrorLog += ANSI2UNICODE(lua_tostring(LuaStack, -1));
					ErrorLog += _T("\r\n");
				}
			}
			return;
		}
	}
}

void ButtonReDraw(Button* bt)
{
	FillRect(dc, &bt->rt, br);
	SetBkColor(dc, RGB(239, 239, 239));
	CString c;
	c.Format(L"%c", bt->KeyVal);
	TextOut(dc, bt->Bise.x, bt->Bise.y, c, c.GetLength());
}

void StickUpdate(POINT p,int id)
{
	for(int i=0;i<StickCount;i++)
	{	
		if(StickList[i]->sid!=id)
			continue;
		if(!(StickList[i]->BitFlag&StickEnb))
			continue;
		if((StickList[i]->BitFlag&StickUse))
		{
			StickList[i]->NPOS=p;
			return;
		}
	}
}
void StickCheckKey(Stick *sk,RECT r)
{
	POINT p;
	if (sk->BitFlag&StickK2M)
	{
		p.x = 0;
		p.y = 0;
		if (r.top<sk->mrt.top)
		{
			p.y = -(((sk->mrt.top - r.top) * 100 / (sk->mrt.top - sk->rt.top))*sk->code[0] / 100);
		}
		if (r.bottom>sk->mrt.bottom)
		{
			p.y = (((r.bottom - sk->mrt.bottom) * 100 / (sk->rt.bottom - sk->mrt.bottom))*sk->code[1] / 100);
		}
		if (r.left<sk->mrt.left)
		{
			p.x = -(((sk->mrt.left - r.left) * 100 / (sk->mrt.left - sk->rt.left))*sk->code[2] / 100);
		}
		if (r.right>sk->mrt.right)
		{
			p.x = (((r.right - sk->mrt.right) * 100 / (sk->rt.right - sk->mrt.right))*sk->code[3] / 100);
		}
		if (p.x != 0 || p.y != 0)
		{
			//p.x = 65536 * p.x / GetSystemMetrics(SM_CXSCREEN);
			//p.y = 65536 * p.y / GetSystemMetrics(SM_CYSCREEN);
			//asnyp = p;
			MouseEvent(p, MOUSEEVENTF_MOVE);
		}
	}
	else
	{
		if(r.top<sk->mrt.top)
		{
			if(!(sk->BitFlag&StickK1))
			{
				StickSetFlag(sk->BitFlag,StickK1);
				KeyEvent(sk->code[0],KEY_DOWN);
			}
		}
		else
		{
			if((sk->BitFlag&StickK1))
			{
				StickClrFlag(sk->BitFlag,StickK1);
				KeyEvent(sk->code[0],KEY_UP);
			}
		}
		if(r.bottom>sk->mrt.bottom)
		{
			if(!(sk->BitFlag&StickK2))
			{
				StickSetFlag(sk->BitFlag,StickK2);
				KeyEvent(sk->code[1],KEY_DOWN);
			}
		}
		else
		{
			if((sk->BitFlag&StickK2))
			{
				StickClrFlag(sk->BitFlag,StickK2);
				KeyEvent(sk->code[1],KEY_UP);
			}
		}
		if(r.left<sk->mrt.left)
		{
			if(!(sk->BitFlag&StickK3))
			{
				StickSetFlag(sk->BitFlag,StickK3);
				KeyEvent(sk->code[2],KEY_DOWN);
			}
		}
		else
		{
			if(sk->BitFlag&StickK3)
			{
				StickClrFlag(sk->BitFlag,StickK3);
				KeyEvent(sk->code[2],KEY_UP);
			}
		}
		if(r.right>sk->mrt.right)
		{
			if(!(sk->BitFlag&StickK4))
			{
				StickSetFlag(sk->BitFlag,StickK4);
				KeyEvent(sk->code[3],KEY_DOWN);
			}
		}
		else
		{
			if((sk->BitFlag&StickK4))
			{
				StickClrFlag(sk->BitFlag,StickK4);
				KeyEvent(sk->code[3],KEY_UP);
			}
		}	
	}
}

void StickDraw(void)
{
	RECT r;
	int dec;
	for(int i=0;i<StickCount;i++)
	{	
		if(!(StickList[i]->BitFlag&StickEnb))
			continue;
		if((StickList[i]->BitFlag&StickUse))
		{
			FillRect(dc,&StickList[i]->rt,br);
			FillRect(dc,&StickList[i]->mrt,br3);
			r.left=StickList[i]->srt.left-StickList[i]->Bise.x+StickList[i]->NPOS.x-StickList[i]->srt.left;
			r.top=StickList[i]->srt.top-StickList[i]->Bise.y+StickList[i]->NPOS.y-StickList[i]->srt.top;;
			r.right=StickList[i]->srt.right-StickList[i]->Bise.x+StickList[i]->NPOS.x-StickList[i]->srt.left;
			r.bottom=StickList[i]->srt.bottom-StickList[i]->Bise.y+StickList[i]->NPOS.y-StickList[i]->srt.top;
			if(r.left<=StickList[i]->rt.left)
			{
				dec=StickList[i]->rt.left-r.left;
				r.left+=dec+1;
				r.right+=dec+1;
			}
			if(r.top<=StickList[i]->rt.top)
			{
				dec=StickList[i]->rt.top-r.top;
				r.top+=dec+1;
				r.bottom+=dec+1;
			}
			if(r.bottom>=StickList[i]->rt.bottom)
			{
				dec=r.bottom-StickList[i]->rt.bottom;
				r.top-=dec+1;
				r.bottom-=dec+1;
			}
			if(r.right>=StickList[i]->rt.right)
			{
				dec=r.right-StickList[i]->rt.right;
				r.right-=dec+1;
				r.left-=dec+1;
			}
			FillRect(dc,&r,br2);
			StickCheckKey(StickList[i],r);
		}
	}
}

void StickDrawAll()
{
	RECT r;
	for(int i=0;i<StickCount;i++)
	{
		if(!(StickList[i]->BitFlag&StickEnb))
			continue;
		FillRect(dc,&StickList[i]->rt,br);
		FillRect(dc,&StickList[i]->mrt,br3);
		r.left=StickList[i]->srt.left+StickList[i]->Pos.x;
		r.top=StickList[i]->srt.top+StickList[i]->Pos.y;
		r.right=StickList[i]->srt.right+StickList[i]->Pos.x;
		r.bottom=StickList[i]->srt.bottom+StickList[i]->Pos.y;
		FillRect(dc,&r,br2);
	}
	for (int i = 0; i < ButtonCount; i++){
		if (!(ButtonList[i]->BitFlag&ButtonEnb))
			continue;
		FillRect(dc, &ButtonList[i]->rt, br);
		SetBkColor(dc, RGB(239, 239, 239));
		CString c;
		c.Format(L"%c", ButtonList[i]->KeyVal);
		TextOut(dc, ButtonList[i]->Bise.x, ButtonList[i]->Bise.y, c, c.GetLength());
	}
}
const char keycode[]={'W','S','A','D'};
Stick *CreateStick(RECT *size,char *code=(char*)keycode,int newflag=0,float bsize=0.4,float dsize=0.5)
{
	if (StickCount >= StickMax)return NULL;
	Stick *sk=new Stick;
	if((size->right-size->left)<50)
		size->right=size->left+50;
	if((size->bottom-size->top)<50)
		size->bottom=size->top+50;
	sk->rt.bottom=size->bottom;
	sk->srt.bottom=(int)((size->bottom-size->top)*0.4)+size->top;
	sk->srt.left=sk->rt.left=size->left;
	sk->rt.right=size->right;
	sk->srt.right=(int)((size->right-size->left)*0.4)+size->left;
	sk->srt.top=sk->rt.top=size->top;
	for(int i=0;i<4;i++)
		sk->code[i]=code[i];
	sk->Pos.x=(sk->rt.right-sk->rt.left)/2-(sk->srt.right-sk->srt.left)/2;
	sk->Pos.y=(sk->rt.bottom-sk->rt.top)/2-(sk->srt.bottom-sk->srt.top)/2;
	sk->Bise.x=(sk->srt.right-sk->srt.left)/2;
	sk->Bise.y=(sk->srt.bottom-sk->srt.top)/2;
	sk->mrt.left=(int)(sk->Pos.x*dsize)+sk->rt.left;
	sk->mrt.right=sk->rt.right-(int)(sk->Pos.x*dsize);
	sk->mrt.top=(int)(sk->Pos.y*dsize)+sk->rt.top;
	sk->mrt.bottom=sk->rt.bottom-(int)(sk->Pos.y*dsize);
	sk->BitFlag = StickEnb | (newflag&0x0f);
	sk->sid=0;
	return sk;
}

Button* CreateButton(RECT *r, int vk,int flag=0, WCHAR *str=NULL){
	Button* bt = new Button;
	bt->rt.bottom = r->bottom;
	bt->rt.top = r->top;
	bt->rt.left = r->left;
	bt->rt.right = r->right;
	bt->KeyVal = vk;
	bt->Bise.x = (r->right - r->left) / 2 - 4+r->left;
	bt->Bise.y = (r->bottom - r->top) / 2 - 8+r->top;
	bt->BitFlag = ButtonEnb | (flag&0xf0);
	bt->sid = 0;
	if (str != NULL)
		bt->str = str;
	else
	{
		bt->str = L"T";
	}
	return bt;
}

bool AddStick(Stick *sk){
	if (StickCount <= StickMax)
	{
		StickList[StickCount] = sk;
		StickCount++;
		return true;
	}
	return false;
}

bool AddButton(Button *bt){
	if (ButtonCount <= ButtonMax)
	{
		ButtonList[ButtonCount] = bt;
		ButtonCount++;
		return true;
	}
	return false;
}

void OnTimer(HWND hWnd)
{
	StickDraw();
	if (ErrorLog.GetLength() > 2)
	{
		MessageBox(g_hWnd, ErrorLog, L"错误", MB_OK);
		ErrorLog = L"";
	}
}

void OnMouseDown(HWND hWnd,POINT p)
{
	//CheckStickDown(p,0);
}

void OnMouseUp(HWND hWnd,POINT p)
{
	//CheckStickUp(p,0);
	//StickDrawAll();
}

void OnMouseMove(HWND hWnd,POINT p)
{
	//StickUpdate(p,0);
}

POINT GetTouchPoint(HWND hWnd,TOUCHINPUT& input)
{
	POINT p;
	p.x=TOUCH_COORD_TO_PIXEL(input.x);
	p.y=TOUCH_COORD_TO_PIXEL(input.y);
	ScreenToClient(hWnd,&p);
	return p;
}

void OnTouchDown(HWND hWnd,TOUCHINPUT& input)
{
	POINT p = GetTouchPoint(hWnd, input);
	CheckStickDown(p, input.dwID);
}

void OnTouchUp(HWND hWnd,TOUCHINPUT& input)
{
	POINT p = GetTouchPoint(hWnd, input);
	CheckStickUp(p, input.dwID);
}

void OnTouchMove(HWND hWnd,TOUCHINPUT& input)
{
	POINT p=GetTouchPoint(hWnd,input);
	StickUpdate(p, input.dwID);
}

MSLLHOOKSTRUCT *pst;
LRESULT CALLBACK MouseProc(int n, WPARAM wParam, LPARAM lParam)
{
	//if (n < 0)
	//	return CallNextHookEx(MouseHook, n, wParam, lParam);
	DWORD a = ((MSLLHOOKSTRUCT*)lParam)->dwExtraInfo & 0xFF515700;
	/*for (int i = 0; i < StickCount; i++){
		if (StickList[i]->BitFlag&StickUse)
			return 1;
	}
	for (int i = 0; i < ButtonCount; i++){
		if (ButtonList[i]->BitFlag&ButtonUse)
			return 1;
	}*/
	if (a == 0xFF515700)
	{
		return 1;
	}
	return CallNextHookEx(MouseHook,n,wParam,lParam);
}

int LuaCreateKeyStick(lua_State *lu){
	int x = lua_tointeger(lu, 1);
	int y = lua_tointeger(lu, 2);
	int size = lua_tointeger(lu, 3);
	const char *s = lua_tostring(lu, 4);
	float bs = (float)lua_tonumber(lu, 5);
	float ps = (float)lua_tonumber(lu, 6);
	if (bs == 0)bs = 0.4;
	if (ps == 0)ps = 0.5;
	if (s == 0)s = keycode;
	RECT r;
	r.left = x;
	r.top = y;
	r.bottom = r.top + size;
	r.right = r.left + size;
	Stick *p = CreateStick(&r, (char*)s, 0, bs, ps);
	int index = StickCount;
	if (AddStick(p))
		lua_pushinteger(lu, StickCount);
	else
		lua_pushinteger(lu, 255);
	return 1; 
}
int LuaCreateMouseStick(lua_State *lu){
	int x = lua_tointeger(lu, 1);
	int y = lua_tointeger(lu, 2);
	int size = lua_tointeger(lu, 3);
	int val = lua_tointeger(lu, 4);
	float bs = (float)lua_tonumber(lu, 5);
	float ps = (float)lua_tonumber(lu, 6);
	if (bs == 0)bs = 0.4;
	if (ps == 0)ps = 0.5;
	if (val == 0)val = 30;
	char buff[4];
	for (int i = 0; i < 4; i++)buff[i] = val;
	RECT r;
	r.left = x;
	r.top = y;
	r.bottom = r.top + size;
	r.right = r.left + size;
	Stick *p = CreateStick(&r, buff, StickK2M, bs, ps);
	int index = StickCount;
	if(AddStick(p))
		lua_pushinteger(lu, index);
	else
		lua_pushinteger(lu, 255);
	return 1;
}
int LuaCreateButton(lua_State *lu){
	int x = lua_tointeger(lu, 1);
	int y = lua_tointeger(lu, 2);
	int ex = lua_tointeger(lu, 3);
	int ey = lua_tointeger(lu, 4);
	int key = lua_tointeger(lu, 5);
	int flag = lua_tointeger(lu, 6);
	if (flag != 0)
	{
		flag = ButtonK2M;
	}
	RECT r;
	r.left = x;
	r.top = y;
	r.bottom = r.top + ey;
	r.right = r.left + ex;
	Button *bt = CreateButton(&r, key, flag);
	int index = ButtonCount;
	if (AddButton(bt))
		lua_pushinteger(lu, index);
	else
		lua_pushinteger(lu, 255);
	return 1;
}

int LuaSetButtonKey(lua_State *lu){
	int id = lua_tointeger(lu, 1);
	int key = lua_tointeger(lu, 2);
	if (ButtonCount <= id)
	{
		lua_pushboolean(lu, false);
		return 1;
	}
	ButtonList[id]->KeyVal = key;
	lua_pushboolean(lu, true);
	ButtonReDraw(ButtonList[id]);
	return 1;
}

int key_down(lua_State *lu)
{
	const char* key = lua_tostring(lu, 1);
	if (key[0] != 0)
		KeyEvent(key[0], 0);
	return 0;
}

int key_up(lua_State *lu)
{
	const char* key = lua_tostring(lu, 1);
	if (key[0] != 0)
		KeyEvent(key[0], 1);
	return 0;
}

int key_press(lua_State *lu)
{
	const char* key = lua_tostring(lu, 1);
	if (key[0] != 0)
		KeyEvent(key[0], 2);
	return 0;
}

int mouse_wheel(lua_State *lu){
	int dat = lua_tointeger(lu, 1);
	MouseEventEx(MOUSEEVENTF_WHEEL, dat);
	return 0;
}

bool doFile(CString file)
{
	CString title;
	char buff[100];
	UNICODE2ANSI(file, buff);
	if (luaL_loadfile(LuaStack, buff))
	{
		//AfxMessageBox(_T("加载错误"));
		ErrorLog += ANSI2UNICODE(lua_tostring(LuaStack, -1));
		ErrorLog += _T("\r\n");
		return true;
	}
	if (lua_pcall(LuaStack, 0, 0, 0))
	{
		//AfxMessageBox(_T("执行错误"));
		//msg(luastack);
		ErrorLog += ANSI2UNICODE(lua_tostring(LuaStack, -1));
		ErrorLog += _T("\r\n");
		return true;
	}
	return false;
}

int msg(lua_State *lu)
{
	const char *s = lua_tostring(lu, -1);
	MessageBox(g_hWnd, ANSI2UNICODE(s), L"提示", MB_OK);
	return 0;
}

const char* ButtonEvent = "local ButtonUp=1\r\nlocal ButtonDown=0\r\nlocal ButtonUPList={}\r\nlocal ButtonDownList={}\r\n\r\nfunction ButtonUpCall(ID)\r\n\r\nif ButtonUPList[ID]==nil then \r\nreturn \r\nend\r\nfor k,i in pairs(ButtonUPList[ID]) do\r\nif i~=nil then \r\ni() \r\nend\r\nend\r\nend\r\n\r\nfunction ButtonDownCall(ID)\r\nif ButtonDownList[ID]==nil then \r\nreturn \r\nend\r\nfor k,i in pairs(ButtonDownList[ID]) do\r\nif i~=nil then \r\ni() \r\nend\r\nend\r\nend\r\n\r\nfunction RegisterButtonEvent(id,fun,event)\r\nif event~=0 then\r\nButtonUPList[id]=ButtonUPList[id] or {}\r\ntable.insert(ButtonUPList[id],fun)\r\nelse\r\nButtonDownList[id]=ButtonDownList[id] or {}\r\ntable.insert(ButtonDownList[id],fun)\r\nend\r\nend\r\n\r\nfunction UnRegisterButtonEvent(id,fun,event)\r\nif event~=0 then\r\nif ButtonUPList[id]==nil then\r\nreturn\r\nend\r\nfor k,i in pairs(ButtonUPList[id]) do\r\nif i==fun then \r\ntable.remove(ButtonUPList[id],k)\r\nend\r\nend\r\nelse\r\nif ButtonDownList[id]==nil then\r\nreturn\r\nend\r\nfor k,i in pairs(ButtonDownList[id]) do\r\nif i==fun then \r\ntable.remove(ButtonDownList[id],k)\r\nend\r\nend\r\nend\r\nend";
void LuaInit(void){
	LuaStack = luaL_newstate();
	luaL_openlibs(LuaStack);
	lua_register(LuaStack, "CreateKeyBoardStick", LuaCreateKeyStick);
	lua_register(LuaStack, "CreateMouseStick", LuaCreateMouseStick);
	lua_register(LuaStack, "CreateButton", LuaCreateButton);
	lua_register(LuaStack, "SetButtonValue", LuaSetButtonKey);
	lua_register(LuaStack, "KeyDown", key_down);
	lua_register(LuaStack, "KeyUp", key_up);
	lua_register(LuaStack, "KeyPress", key_press);
	lua_register(LuaStack, "MouseWheel", mouse_wheel);
	lua_register(LuaStack, "MessageBox", msg);
	if (luaL_dostring(LuaStack, ButtonEvent))
	{
		MessageBox(NULL, L"按键事件加载失败", L"错误", MB_OK);
	}
}

void LuaReset(void){
	lua_close(LuaStack);
	LuaInit();
}


int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPTSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
 	// TODO: 在此放置代码。
	MSG msg;
	// 初始化全局字符串
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_WINDOWTEST, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);
	
	 //执行应用程序初始化:
	//if (!InitInstance (hInstance, nCmdShow))
	//{
	//	return FALSE;
	//}
	//hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_WINDOWTEST));
	HWND hWnd=CreateWindowEx(WS_EX_LAYERED|WS_EX_TOOLWINDOW|WS_EX_NOACTIVATE,szWindowClass,L"T2",WS_SYSMENU|WS_POPUP,0,0,400,400,NULL, NULL, hInstance, NULL);
	SetLayeredWindowAttributes(hWnd,RGB(255,255,255),200,3);
	SetWindowPos(hWnd,HWND_TOPMOST,0,0,GetSystemMetrics(SM_CXSCREEN),GetSystemMetrics(SM_CYSCREEN),SWP_SHOWWINDOW|SWP_NOACTIVATE);
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);
	dc=GetDC(hWnd);
	br=CreateSolidBrush(RGB(239,239,239));
	br2=CreateSolidBrush(RGB(208,208,208));
	br3=CreateSolidBrush(RGB(179,170,179));
	g_hWnd = hWnd;
	LuaInit();
	
	//if (FileExist("main.lua", 0))
	if (doFile("main.lua")){
		MessageBox(hWnd, ErrorLog, L"脚本错误", MB_OK);
		return true;
	}
	MouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseProc, GetModuleHandle(NULL), 0);
	/*RECT r;
	r.left=100;
	r.right=250;
	r.top=550;
	r.bottom=700;
	const char b[4]={'W','S','A','D'};
	Stick *sk=CreateStick(&r,(char*)b);
	StickList[StickCount] = sk;
	StickCount++;

	r.left = 100;
	r.right = 250;
	r.top = 350;
	r.bottom = 500;
	const char c[4] = { 30, 30, 30, 30 };
	sk = CreateStick(&r, (char*)c,StickK2M);
	StickList[StickCount] = sk;
	StickCount++;

	r.left = 1000;
	r.right = 1100;
	r.top = 600;
	r.bottom = 700;
	Button *bt = CreateButton(&r, 'Z');
	ButtonList[ButtonCount] = bt;
	ButtonCount++;
	r.left = 1000;
	r.right = 1100;
	r.top = 500;
	r.bottom = 590;
	bt = CreateButton(&r, 'X');
	ButtonList[ButtonCount] = bt;
	ButtonCount++;
	r.left = 1000;
	r.right = 1100;
	r.top = 400;
	r.bottom = 490;
	bt = CreateButton(&r, 'C');
	ButtonList[ButtonCount] = bt;
	ButtonCount++;
	r.left = 1000;
	r.right = 1100;
	r.top = 300;
	r.bottom = 390;
	bt = CreateButton(&r, 'V');
	ButtonList[ButtonCount] = bt;
	ButtonCount++;
	r.left = 1110;
	r.right = 1200;
	r.top = 300;
	r.bottom = 700;
	bt = CreateButton(&r, ' ');
	ButtonList[ButtonCount] = bt;
	ButtonCount++;*/
	StickDrawAll();
	SetTimer(hWnd,0,50,NULL);
	if(!RegisterTouchWindow(hWnd,0))
	{
		MessageBox(hWnd,L"无法注册触摸事件",L"提示",MB_OK);
	}
	// 主消息循环:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, NULL, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int) msg.wParam;
}



//
//  函数: MyRegisterClass()
//
//  目的: 注册窗口类。
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_WINDOWTEST));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= NULL;
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}
//
//  函数: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目的: 处理主窗口的消息。
//
//  WM_COMMAND	- 处理应用程序菜单
//  WM_PAINT	- 绘制主窗口
//  WM_DESTROY	- 发送退出消息并返回
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case WM_TIMER:
		OnTimer(hWnd);
		break;
	case WM_MOUSEMOVE:
		int nflag;
		static int saveflag;
		nflag=wParam;
		POINT p;
		p.x=LOWORD(lParam);
		p.y=HIWORD(lParam);
		if((saveflag&MK_LBUTTON)!=(nflag&MK_LBUTTON))
		{
			saveflag=nflag;
			if(nflag&MK_LBUTTON)
			{	
				OnMouseDown(hWnd,p);
			}
			else
			{
				OnMouseUp(hWnd,p);
			}
		}
		OnMouseMove(hWnd,p);
		break;
	case WM_TOUCH:
		TouchPointCount=(int)wParam;
		if(GetTouchInputInfo((HTOUCHINPUT)lParam,TouchPointCount,TouchBuffPrt,sizeof(TOUCHINPUT)))
		{
			DWORD Flag;
			for(int i=0;i<TouchPointCount;i++)
			{
				Flag=TouchBuff[i].dwFlags&0x0007;
				switch(Flag)
				{
				case TOUCHEVENTF_MOVE:
					OnTouchMove(hWnd,TouchBuff[i]);
					break;
				case TOUCHEVENTF_DOWN:
					OnTouchDown(hWnd,TouchBuff[i]);
					break;
				case TOUCHEVENTF_UP:
					OnTouchUp(hWnd,TouchBuff[i]);
					break;
				}
			}
			CloseTouchInputHandle((HTOUCHINPUT)lParam);
		}
		break;
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// 分析菜单选择:
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		// TODO: 在此添加任意绘图代码...
		StickDrawAll();
		SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), SWP_SHOWWINDOW | SWP_NOACTIVATE);
		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		UnhookWindowsHookEx(MouseHook);
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// “关于”框的消息处理程序。
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}
