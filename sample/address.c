#include "../include/common.h"
#include "../include/lguibase.h"

#define  ID_BUTTON 100
#define  ID_BUTTON2 200
#define  ID_NEWWIN	101
#define  ID_SUB_BUTTON 102
#define  ID_SUB_NEWWIN	103



LRESULT WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT ChildWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

int CreateSubWindow(HWND hWndParent);

int WinMain()
{

	int iStatus;
	HANDLE hWnd;
	int i=0;
	WNDCLASSEX wcex;
	MSG msg;


	if(!InitGUIClient())
		return 0;


	wcex.cbSize			= sizeof(WNDCLASSEX);
	wcex.style			= 0;
	wcex.lpfnWndProc	= (WNDPROC)WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= 0;
	wcex.hIcon			= 0;
	wcex.hCursor		= 0;
	wcex.hbrBackground	= CreateSolidBrush(RGB(147,222,252));
	wcex.lpszMenuName	= NULL;
	wcex.lpszClassName	= "mainwin";
	wcex.hIconSm		= 0;

	RegisterClass(&wcex);

	hWnd = CreateWindow("mainwin", "main", WS_MAIN | WS_VISIBLE | 
			WS_THINBORDER|WS_CAPTION | WS_OKBUTTON |WS_CLOSEBOX,
			 70, 20,140, 180, NULL, NULL, NULL, NULL);


	if (!hWnd)  return false;
	ShowWindow(hWnd, true);
	UpdateWindow(hWnd);


	while (GetMessage(&msg,hWnd)){
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	TerminateGUIClient();
}

LRESULT WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HDC hDC;
	static RECT rc;
	static PAINTSTRUCT ps;
	static HWND hWndButton,hWndbutton2,hWndNewWin;
	int x,y;
	switch(message)
	{
		case LMSG_CREATE:
			hWndNewWin = CreateWindow("mledit", "AAAABBBBCCCCDDDDEEEEFFFFGGGG", WS_CONTROL | WS_BORDER | WS_VISIBLE
				| ES_MULTILINE | ES_AUTOWRAP,
				10, 5, 100 , 65, hWnd, (HMENU)ID_BUTTON2, NULL, NULL);

			hWndButton = CreateWindow("button", "Close", WS_CONTROL  | BS_PUSHBUTTON | WS_BORDER | WS_VISIBLE,
				20, 110, 80 , 20, hWnd, (HMENU)ID_BUTTON, NULL, NULL);

			hWndbutton2 = CreateWindow("button", "NewWin", WS_CONTROL  | BS_PUSHBUTTON | WS_BORDER | WS_VISIBLE,
				20, 80, 80 , 20, hWnd, (HMENU)ID_NEWWIN, NULL, NULL);
			break;
		case LMSG_COMMAND:
			switch(HIWORD(wParam)){
			case BN_CLICKED:

				switch(LOWORD(wParam)){
				case ID_BUTTON:
					DestroyWindow(hWnd);
					break;
				case ID_NEWWIN:
					CreateSubWindow(hWnd);
					break;
				default:
					break;
				}
				break;
			default:
				break;
			}
			break;
		case LMSG_PENDOWN:
			hDC=GetDC(hWnd);
			x=(int)wParam;
			y=(int)lParam;
			ScreenToClient(hWnd,&x,&y);
			SetPixel(hDC,x,y ,RGB(0,0,0));
			ReleaseDC(hWnd,hDC);

			CaptureMouse(hWnd,BYCLIENT);
			break;
		case LMSG_PENMOVE:
			hDC=GetDC(hWnd);
			x=(int)wParam;
			y=(int)lParam;
			ScreenToClient(hWnd,&x,&y);
			SetPixel(hDC,x,y ,RGB(0,0,0));
			ReleaseDC(hWnd,hDC);
			break;
		case LMSG_PENUP:
			DisCaptureMouse();
			break;
		case LMSG_PAINT:
			ps.bPaintDirect=false;

			hDC=BeginPaint(hWnd, &ps);
			if(!hDC){
				return true;
			}
		//	ShowBitmap(hDC,0,0,"/usr/local/lgui/temp/record.bmp");
			EndPaint(hWnd, &ps);
			break;
		case LMSG_CLOSE:
			PostQuitMessage(hWnd);//�����˳���Ϣѭ��
		case LMSG_DESTROY:
			PostQuitMessage(hWnd);//�����˳���Ϣѭ��
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return true;
}


int CreateSubWindow(HWND hWndParent)
{
	int iStatus;
	HANDLE hWnd;
	WNDCLASSEX wcex;
	MSG msg;
	SCROLLINFO si;

	wcex.cbSize			= sizeof(WNDCLASSEX);
	wcex.style			= 0;
	wcex.lpfnWndProc	= (WNDPROC)ChildWndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= 0;
	wcex.hIcon			= 0;
	wcex.hCursor		= 0;
	wcex.hbrBackground	= CreateSolidBrush(RGB(200,100,20));
	wcex.lpszMenuName	= NULL;
	wcex.lpszClassName	= "childwin";
	wcex.hIconSm		= 0;

	RegisterClass(&wcex);

	hWnd = CreateWindow("childwin", "child", WS_CHILD | WS_VISIBLE | WS_THINBORDER ,
		10, 20,100, 100, hWndParent, NULL, NULL, NULL);
	if (!hWnd)
		return false;

	ShowWindow(hWnd, true);

	return true;
}


LRESULT ChildWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HDC hDC;
	static RECT rc;
	static PAINTSTRUCT ps;
	static HWND hWndButton,hWndNewWin;
	int x,y;
	switch(message)
	{
		case LMSG_CREATE:
			hWndButton = CreateWindow("button", "Close", WS_CONTROL  | BS_PUSHBUTTON | WS_BORDER | WS_VISIBLE,
				40, 60, 40 , 20, hWnd, (HMENU)ID_SUB_BUTTON, NULL, NULL);
			break;
		case LMSG_COMMAND:
			switch(HIWORD(wParam)){
			case BN_CLICKED:
				switch(LOWORD(wParam)){

				case ID_SUB_BUTTON:
					DestroyWindow(hWnd);
					break;
				default:
					break;
				}
				break;
			default:
				break;
			}

			break;
		case LMSG_PENDOWN:

			CaptureMouse(hWnd,BYCLIENT);
			break;
		case LMSG_PENMOVE:
			hDC=GetDC(hWnd);
			x=(int)wParam;
			y=(int)lParam;
			ScreenToClient(hWnd,&x,&y);
			SetPixel(hDC,x,y ,RGB(0,0,0));
			ReleaseDC(hWnd,hDC);
			break;
		case LMSG_PENUP:
			DisCaptureMouse();
			break;
		case LMSG_PAINT:
			ps.bPaintDirect=false;
			hDC=BeginPaint(hWnd, &ps);
			if(!hDC){
				return true;
			}
			EndPaint(hWnd, &ps);
			break;
		case LMSG_DESTROY:
			PostQuitMessage(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return true;
}






