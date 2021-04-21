#include "../source/include/common.h"
#include "../source/include/lguibase.h"


BOOL _lGUI_bAllowPaint=true;//是否允许绘制

LRESULT WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
#define  ID_BUTTON 100
#define  ID_BUTTON1 101
int main()
{

	int iStatus;
	HANDLE hWnd;
//	HBRUSH hBrush;
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

	hWnd = CreateWindow("mainwin", "main", WS_MAIN | WS_VISIBLE | WS_THINBORDER,
			 10, 100,150, 80, NULL, NULL, NULL, NULL);


	if (!hWnd)  return false;
	ShowWindow(hWnd, true);
	//UpdateWindow(hWnd);


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
	static HWND hWndButton,hWnd1;
	int x,y;

	switch(message)
	{
		case LMSG_CREATE:
			hWndButton = CreateWindow("button", "Close", WS_CONTROL  | WS_BORDER | WS_VISIBLE,
				70, 20, 80 , 20, hWnd, (HMENU)ID_BUTTON, NULL, NULL);
			//hWnd1 = CreateWindow("button", "hide", WS_CONTROL  |BS_PUSHBUTTON | WS_BORDER | WS_VISIBLE,
			//	20, 50, 80 , 20, hWnd, (HMENU)ID_BUTTON, NULL, NULL);
			hWnd1 = CreateWindow("button", "Close", WS_CONTROL  | WS_BORDER | WS_VISIBLE,
				70, 50, 80 , 20, hWnd, (HMENU)ID_BUTTON1, NULL, NULL);

			break;
		case LMSG_COMMAND:
			switch(HIWORD(wParam)){
			case BN_CLICKED://桌面图标的点击事件处理

				switch(LOWORD(wParam)){

				case ID_BUTTON:
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
		case LMSG_PENDOWN://桌面的mouse事件
			//MoveWindow(hWnd1,2,2);
			break;
		case LMSG_PENMOVE:
			hDC=GetDC(hWnd);
			x=(int)wParam;
			y=(int)lParam;
//			printf("%d,%d\n",x,y);
			ScreenToClient(hWnd,&x,&y);
			SetPixel(hDC,x,y ,RGB(0,0,0));
			ReleaseDC(hWnd,hDC);


			break;
		case LMSG_PENUP://桌面的mouse事件
			break;
		case LMSG_PAINT:
			ps.bPaintDirect=false;

			hDC=BeginPaint(hWnd, &ps);
			if(!hDC){
				return true;
			}
			//ShowBitmap(hDC,0,0,"/usr/local/lgui/temp/record.bmp");
			EndPaint(hWnd, &ps);
			break;
		case LMSG_DESTROY:
			PostQuitMessage(hWnd);//用来退出消息循环
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return true;
}
