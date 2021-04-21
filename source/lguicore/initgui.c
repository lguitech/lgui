/*
	Copyright (C) 2004-2005 Li Yudong
*/
/*
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "../include/common.h"
#include "../include/blockheap.h"
#include "../include/mouse.h"
#include "../include/keyboard.h"
#include "../include/framebuffer.h"
#include "../include/rect.h"
#include "../include/invalidregion.h"
#include "../include/clipregion.h"
#include "../include/hdc.h"
#include "../include/lguiapp.h"
#include "../include/ipcsocket.h"
#include "../include/message.h"
#include "../include/regclass.h"
#include "../include/shmem.h"
#include "../include/caret.h"
#include "../include/winnc.h"
#include "../include/winbase.h"
#include "../include/window.h"
#include "../include/scrollbar.h"
#include "../include/ipcsignal.h"
#include "../include/timer.h"
#include "../include/initgui.h"


extern PlGUIAppStat  _lGUI_pAppStat;
extern WndCaptureMouse _lGUI_wndCaptureMouse;


BOOL GUIAPI
InitGUIServer()
{
	RegisterServerControls();

	InitMsgQueueHeap();
	InitClipRegionHeap();
	InitInvalidRegionHeap();

	if(!InitFrameBuffer()){
		printerror("init framebuffer error!");
		return false;
	}

	InitShareMemServer();

	CreateStockObject();

	if(!InitIpcSocketServer())
		return false;
	RegisterTimerRoutine();

	InitMouseServer();

	InitKeyboard();
	return true;
}


BOOL GUIAPI 
InitGUIClient()
{
	RegisterClientControls();

	InitMsgQueueHeap();
	InitClipRegionHeap();
	InitInvalidRegionHeap();


	if(!InitFrameBuffer()){
		printerror("init framebuffer error!");
		return false;
	}

	_lGUI_pAppStat=(PlGUIAppStat)malloc(sizeof(lGUIAppStat));
	if(!_lGUI_pAppStat){
		printerror("alloc memory error!");
		return false;
	}
	memset(_lGUI_pAppStat,0,sizeof(lGUIAppStat));
	_lGUI_pAppStat->pClipRgn=(PClipRegion)malloc(sizeof(ClipRegion));
	if(!(_lGUI_pAppStat->pClipRgn))
		return false;
	memset(_lGUI_pAppStat->pClipRgn,0,sizeof(ClipRegion));

	InitClipRegion(_lGUI_pAppStat->pClipRgn);


	if(!InitIpcSocketClient())
		return false;

	InitShareMemClient();
	//register signal callback function, used when server want to terminate client 
	RegSignalCallBack();
	//initial timer function
	RegisterTimerRoutine();

	InitMouseClient();

	_lGUI_wndCaptureMouse.pWin=NULL;
	return true;
}


void GUIAPI 
TerminateGUIServer()
{

	TerminateMouseServer();
	TerminateKeyboard();
	TerminateIpcSocket();
	DestroyRegWndTable();
	UnInitFrameBuffer();

	DestroyClipRegionHeap();
	DestroyInvalidRegionHeap();
	DestroyMsgQueueHeap();

	UnInitShareMem();
	UnInitTimer();
}

void GUIAPI 
TerminateGUIClient()
{
	//main window clipregion maintained by server end.
	EmptyClipRegion(_lGUI_pAppStat->pClipRgn);
	free(_lGUI_pAppStat->pClipRgn);

	free(_lGUI_pAppStat);

	DestroyRegWndTable();

	DestroyClipRegionHeap();
	DestroyInvalidRegionHeap();
	DestroyMsgQueueHeap();

	UnInitShareMem();
	TerminateIpcSocket();

	//cancel the socket thread will close the socket at first
	//if client close the socket,server will get this socket close event
	//and will process destroy application.
	UnInitTimer();
}
