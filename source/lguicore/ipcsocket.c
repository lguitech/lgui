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
#include "../include/socketio.h"
#include "../include/blockheap.h"
#include "../include/rect.h"
#include "../include/invalidregion.h"
#include "../include/clipregion.h"
#include "../include/message.h"
#include "../include/hdc.h"
#include "../include/caret.h"
#include "../include/winnc.h"
#include "../include/winbase.h"
#include "../include/window.h"
#include "../include/lguiapp.h"
#include "../include/socketio.h"
#include "../include/ipcsocket.h"
#include "../include/mouse.h"


extern PWindowsTree		_lGUI_pActiveWin;
extern PWindowsTree		_lGUI_pFocus;
extern PlGUIAppStat		_lGUI_pAppStat;
extern PWindowsTree 	_lGUI_pWindowsTree;
extern WndCaptureMouse _lGUI_wndCaptureMouse;
extern PlGUIAppStat _lGUI_pAppStat;
extern PWindowsTree _lGUI_pImeWindow;
extern PWindowsTree _lGUI_pSkbWindow;

//two global variable
//listen thread for server
//connect thread for client

pthread_t 		thread_ipcmsg;
//identify of listen socket for server
//identify of connect socket for client

int				_lGUI_fdSocket;




void 
TerminateIpcSocket()
{
	pthread_cancel(thread_ipcmsg);
	pthread_join(thread_ipcmsg,NULL);
}


//thread cleanup function
//register to do cleanup work while thread exit
void 
cleanup_after_malloc(
	void* allocated_memory
)
{
	 if(allocated_memory)
		free(allocated_memory);
}

void 
cleanup_after_opensock(
	void* fd
)
{
	int t;
	t=*((int*)fd);
	close(t);
}

//===============================================================================
//****************************Server*********************************************
//===============================================================================


//Initial listen socket and create thread for server
BOOL
InitIpcSocketServer()
{
	_lGUI_fdSocket=serv_listen(CS_PATH);
	if(_lGUI_fdSocket<0){
		printerror("server socket listen return error!");
		return false;
	}
	pthread_create(&thread_ipcmsg,NULL,IpcSocketMainLoopServer,(void*)&_lGUI_fdSocket);
	return true;
}

//main loop for listen thread
void* 
IpcSocketMainLoopServer(
	void* para
)
{
	PlGUIAppStat pStat;
	int clifd;
	int fd;
	fd=*((int*)para);

	while(1){
		pthread_testcancel();
		clifd=serv_accept(fd);
		if(clifd!=-1){
			pStat=(PlGUIAppStat)malloc(sizeof(lGUIAppStat));
			if(!pStat)
				continue;
			memset(pStat,0,sizeof(lGUIAppStat));

			pStat->fdSocket = clifd;

			if(!_lGUI_pAppStat)
				_lGUI_pAppStat=pStat;
			else{
				pStat->pNext=_lGUI_pAppStat;
				_lGUI_pAppStat=pStat;
			}
			//create socket thread. A thread for a application 
			pthread_create(&(_lGUI_pAppStat->tdSocket),NULL,(void*)ReadMainLoopServer,(void*)&clifd);
		}
	}
}

//Main loop for socket read
void* 
ReadMainLoopServer(
	void* clifd
)
{
	void* pData;
	int iSize;
	int iRet;
	int fd;
	int old_cancel_type;

	int iMsg;

	fd=*((int*)clifd);
	pData=malloc(SIZE_IPC_MSG);
	if(!pData)
		pthread_exit(0);
	//register thread cleanup function
	pthread_cleanup_push(cleanup_after_malloc,pData);
	while(1){
		//read value of message at first£»
		iRet = sock_read(fd,(void*)&iMsg,sizeof(int));
		if(iRet==SOCKERR_CLOSED || iRet==SOCKERR_IO){
			//printerror("socket error while reading type of message!");
			ProcessIpcMessageServer(fd,LMSG_IPC_DESTROYAPP,NULL,0);
			pthread_exit(0);
		}
		//read the size of added data 
		iRet=sock_read(fd,(void*)&iSize,sizeof(int));

		if(iRet==SOCKERR_CLOSED || iRet==SOCKERR_IO){
			//printerror("socket error while reading size of message!");
			ProcessIpcMessageServer(fd,LMSG_IPC_DESTROYAPP,NULL,0);
			pthread_exit(0);
		}
		//read added data
		iRet=sock_read(fd,pData,iSize);
		if(iRet==SOCKERR_CLOSED || iRet==SOCKERR_IO){
			//printerror("socket error while reading message!");
			ProcessIpcMessageServer(fd,LMSG_IPC_DESTROYAPP,NULL,0);
			pthread_exit(0);
		}
		//process the readed message
		ProcessIpcMessageServer(fd,iMsg,pData,iSize);
	}
	pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED,&old_cancel_type);
	pthread_cleanup_pop(1);
	pthread_setcanceltype(old_cancel_type,NULL);
}

//Process ipc message for sersver
BOOL 
ProcessIpcMessageServer(
	int fd, 
	int iMsg,  
	void* pAttachData, 
	int iAttachSize
)
{
	switch (iMsg){
		case LMSG_IPC_CREATEAPP:
			//create application
			CreateApplicationServer(fd, pAttachData, iAttachSize);
			break;
		case LMSG_IPC_DESTROYAPP:
			//if two thread a and b are siblings.thread a can not
			// wait on any processes forked by thread b
			// so i have to send a notify message to main thread
			//Destroy application
			SendNotifyMessage((HWND)_lGUI_pWindowsTree,LMSG_IPC_DESTROYAPP,(WPARAM)fd,(LPARAM)NULL);
			//DestroyApplicationServer(fd, pAttachData, iAttachSize);
			break;
		case LMSG_IPC_SHOWMAINWIN:
			//Show Main Window
			ShowMainWinServer(fd, pAttachData, iAttachSize);
			break;
		case LMSG_IPC_HIDEMAINWIN:
			//Hide Main Window
			HideMainWinServer(fd, pAttachData, iAttachSize);
			break;
	}
	return true;
}


BOOL 
CreateApplicationServer(
	int fd, 
	void* pAttachData, 
	int iAttachSize
)
{
	PlGUIAppStat pStat;
	PlGUIAppStat pCurStat;

	pStat = _lGUI_pAppStat;
	if(!pStat)
		return false;

	while(pStat){
		if(pStat->fdSocket == fd)
			break;
		pStat = pStat->pNext;
	}
	if(!pStat)
		return false;
	pCurStat = (PlGUIAppStat)pAttachData;
	if(!pCurStat)
		return false;
	strcpy(pStat->pAppName,pCurStat->pAppName);
	CopyRect(&(pStat->rc),&(pCurStat->rc));
	pStat->bVisible=false;//it's invisible before show operate
	//init clip area
	pStat->pClipRgn=(PClipRegion)malloc(sizeof(ClipRegion));
	if(!(pStat->pClipRgn))
		return false;
	memset(pStat->pClipRgn,0,sizeof(ClipRegion));
	InitClipRegion(pStat->pClipRgn);
	return true;
}
//Destroy an application
//socket id
// pAttachData iAttachSize no used 
BOOL 
DestroyApplicationServer(
	int fd, 
	void* pAttachData, 
	int iAttachSize
)
{
	PWindowsTree pControl;
	PlGUIAppStat pStat,pCurStat,pPrevStat=NULL;
	int status;

	BOOL bTop=false;

	pStat = _lGUI_pAppStat;

	if(!pStat)
		return false;
	while(pStat){
		if(pStat->fdSocket==fd){
			break;
		}
		pPrevStat=pStat;
		pStat=pStat->pNext;
	}
	if(!pStat){
		return false;
	}
	if(pStat == _lGUI_pAppStat){
		bTop = true;
		_lGUI_pAppStat = _lGUI_pAppStat->pNext;
	}
	else
		pPrevStat->pNext = pStat->pNext;

	//wait to clean the child process image in kernal
	//otherwise the child process will become to be a defunct process

	//pthread_cancel(pStat->tdSocket);
	//pthread_join(pStat->tdSocket,NULL);

	waitpid((pid_t)((atoi)(pStat->pAppName)),&status,0);
	if(pStat->bVisible){//only if this application is visible
		if(pStat->pNext){
			//recalculate clip region of mainwindow under the current
			RecalClipRgnUnderThis(pStat->pNext);
			//send clip region change message to application
			pCurStat=pStat->pNext;
			while(pCurStat){
				SendClipRgnChangeMsg(pCurStat);
				pCurStat=pCurStat->pNext;
			}

			//if this application is at top layer
			// then we should active the nearest application under it
			if(bTop){
				SendMsgByServer(_lGUI_pAppStat,LMSG_IPC_ACTIVEAPP,NULL,0);
			}
			//send redraw message to application
			pCurStat=pStat->pNext;
			while(pCurStat){
				SendMsgByServer(pCurStat,LMSG_IPC_REDRAW,(void*)&(pStat->rc),sizeof(RECT));
				pCurStat=pCurStat->pNext;
			}
		}

		//recalculate clip region of desktop
		ReCalClipRegion(_lGUI_pWindowsTree);
		//recalculate clip region of desktop control
		pControl=_lGUI_pWindowsTree->pControlHead;
		while(pControl){
			ReCalClipRegion(pControl);
			pControl=pControl->pNext;
		}
		//redraw desktop 
		scrInvalidateRect((HWND)_lGUI_pWindowsTree,&(pStat->rc),true);
		//redraw desktop control
		pControl=_lGUI_pWindowsTree->pControlHead;
		while(pControl){
			if(IsVisible(pControl))
				scrInvalidateRect((HWND)pControl,&(pStat->rc),true);
			pControl=pControl->pNext;
		}
		//if all of application have been destroyed,we should active desktop.
		if(!_lGUI_pAppStat){
			ActiveWindow(_lGUI_pWindowsTree);
		}
	}
	//destroy the application
	EmptyClipRegion(pStat->pClipRgn);
	free(pStat->pClipRgn);

	free(pStat);

	return true;
}



//Show Main Window
//para:
// socket id
// pAttachData iAttachSize no used 
BOOL 
ShowMainWinServer(
	int fd, 
	void* pAttachData, 
	int iAttachSize
)
{
	PlGUIAppStat pStat,pCurStat,pPrevStat,pThisStat,pActiveStat;
	PWindowsTree pControl;
	RECT rect;
	
	if(IsActive((HWND)_lGUI_pWindowsTree)){//if desktop is active
		pActiveStat = NULL;
	}
	else{
		pStat=_lGUI_pAppStat;
		while(pStat){
			if(pStat->bVisible)
				break;
			pStat = pStat->pNext;
		}
		pActiveStat = pStat;
	}

	pStat = _lGUI_pAppStat;
	pPrevStat = _lGUI_pAppStat;
	while(pStat){
		if(pStat->fdSocket==fd)
			break;
		pPrevStat = pStat;
		pStat=pStat->pNext;
	}
	if(!pStat)
		return false;
	//Move this main window node to top
	if(pStat!=_lGUI_pAppStat){
		pThisStat = pStat->pNext;

		pPrevStat->pNext = pStat->pNext;
		pStat->pNext = _lGUI_pAppStat;
		_lGUI_pAppStat = pStat;
	}
	else
		pThisStat = NULL;

	pStat->bVisible=true;
	
	//clipped by desktop rect
	GetWindowRect((HWND)_lGUI_pWindowsTree,&rect);
	IntersectRect(&(pStat->rc),&(pStat->rc),&rect);
	//Initial the clip region 
	SetInitRectClipRegion (pStat->pClipRgn, &(pStat->rc));
	//clipped by imewin and skbwin to generate initial clip region
	if(IsVisible((HWND)_lGUI_pImeWindow))
		SubtractClipRegion(pStat->pClipRgn,&_lGUI_pImeWindow->rect);
	if(IsVisible((HWND)_lGUI_pSkbWindow))
		SubtractClipRegion(pStat->pClipRgn,&_lGUI_pSkbWindow->rect);
	//clip region of all main window under this will change.
	pCurStat=_lGUI_pAppStat->pNext;
	while(pCurStat!= pThisStat){
		if(IsIntersect(&(pCurStat->pClipRgn->rcBound),&(pStat->rc))){
			SubtractClipRegion (pCurStat->pClipRgn, &(pStat->rc));
			//send clipregion change message to application
			if(pCurStat->bVisible)
				SendClipRgnChangeMsg(pCurStat);
		}
		pCurStat = pCurStat->pNext;
	}

	//send disactive message to the top application before.
	if(pActiveStat){
		SendMsgByServer(pActiveStat,LMSG_IPC_DISACTIVEAPP,NULL,0);
	}

	//calculate clipregion of desktop
	SubtractClipRegion(_lGUI_pWindowsTree->pClipRgn,&(pStat->rc));
	//calculate clip region of desktop control 
	pControl=_lGUI_pWindowsTree->pControlHead;
	while(pControl){
		SubtractClipRegion(pControl->pClipRgn,&(pStat->rc));
		pControl=pControl->pNext;
	}
	//send confirm message to the new generated application
	SendShowMainWinAnsToClient(_lGUI_pAppStat);
	return true;
}
//send new generaged clip region to confirm the client waitting process
//the content sent by this message is same like send clip change message
void 
SendShowMainWinAnsToClient(
	PlGUIAppStat pStat
)
{
	PClipRect pHead,pTail;
	char* pAttachData;
	int iAttachSize;
	char* pCurPointer;
	int iCount;

	pHead=pStat->pClipRgn->pHead;
	pTail=pStat->pClipRgn->pTail;
	if(!pHead)
		iCount=0;
	else{
		iCount=1;
		while(pHead!=pTail){
			iCount++;
			pHead=pHead->pNext;
		}
	}
	if(iCount){
		iAttachSize=iCount*sizeof(RECT);
		pAttachData=malloc(iAttachSize);
		pCurPointer=pAttachData;
		pHead=pStat->pClipRgn->pHead;
		while(pHead){
			memcpy((void*)pCurPointer,&(pHead->rect),sizeof(RECT));

			pCurPointer+=sizeof(RECT);
			pHead=pHead->pNext;
		}
		SendMsgByServer(pStat,LMSG_IPC_SHOWMAINWIN_ANS,pAttachData,iAttachSize);
		free(pAttachData);
	}
}

//Hide main window
//Param
//  socket id
//  pAttachData iAttachSize no used 
BOOL 
HideMainWinServer(
	int fd, 
	void* pAttachData, 
	int iAttachSize
)
{
	PlGUIAppStat pStat,pCurStat;
	PWindowsTree pControl;

	pStat=_lGUI_pAppStat;
	while(pStat){
		if(pStat->fdSocket==fd)
			break;
		pStat=pStat->pNext;
	}
	if(!pStat)
		return false;

	pStat->bVisible=false;

	ReCalClipRegion(_lGUI_pWindowsTree);
	InvalidateRect((HWND)_lGUI_pWindowsTree,NULL,true);

	pControl=_lGUI_pWindowsTree->pControlHead;

	while(pControl){
		ReCalClipRegion(pControl);
		pControl=pControl->pNext;
	}
	pControl=_lGUI_pWindowsTree->pControlHead;
	while(pControl){
		scrInvalidateRect((HWND)pControl,NULL,true);
		pControl=pControl->pNext;
	}

	RecalClipRgnUnderThis(pStat->pNext);
	pCurStat=pStat->pNext;
	while(pCurStat){
		SendClipRgnChangeMsg(pCurStat);
		pCurStat=pCurStat->pNext;
	}
	pCurStat=pStat->pNext;
	while(pCurStat){
		SendMsgByServer(pCurStat,LMSG_IPC_REDRAW,(void*)&(pStat->rc),sizeof(RECT));
		pCurStat=pCurStat->pNext;
	}
	return true;
}


//send message to client
//parameter:
//1. application description pointer
//2. message type
//3. message added data
//4. size of added message data
BOOL 
SendMsgByServer(
	PlGUIAppStat pStat,
	int iMsg,
	void* pAttachData,
	int iAttachSize
)
{
	if(!pStat->fdSocket)
		return false;
	//send message value
	sock_write(pStat->fdSocket,(void*)&iMsg,sizeof(int));
	//send size of added data 
	sock_write(pStat->fdSocket,(void*)&iAttachSize,sizeof(int));

	if(pAttachData && iAttachSize)
		//send added data
		sock_write(pStat->fdSocket,pAttachData,iAttachSize);

	return true;

}



//send clip region change message 
void 
SendClipRgnChangeMsg(
	PlGUIAppStat pStat
)
{
	PClipRect pHead,pTail;
	char* pAttachData;
	int iAttachSize;
	char* pCurPointer;
	int iCount;

	pHead=pStat->pClipRgn->pHead;
	pTail=pStat->pClipRgn->pTail;
	if(!pHead)
		iCount=0;
	else{
		iCount=1;
		while(pHead!=pTail){
			iCount++;
			pHead=pHead->pNext;
		}
	}
	if(iCount){
		iAttachSize=iCount*sizeof(RECT);
		pAttachData=malloc(iAttachSize);
		pCurPointer=pAttachData;
		pHead=pStat->pClipRgn->pHead;
		while(pHead){
			memcpy((void*)pCurPointer,&(pHead->rect),sizeof(RECT));
			pCurPointer+=sizeof(RECT);
			pHead=pHead->pNext;
		}
		SendMsgByServer(pStat,LMSG_IPC_CLIPRGNCHANGE,pAttachData,iAttachSize);
		free(pAttachData);
	}

}
//recalculate clip regioin of all application main window under current window 
void 
RecalClipRgnUnderThis(
	const PlGUIAppStat pStat
)
{
	PlGUIAppStat pCurStat,pPrevStat;
	pCurStat=pStat;
	while(pCurStat){
		SetInitRectClipRegion(pCurStat->pClipRgn,&pCurStat->rc);

		pPrevStat=_lGUI_pAppStat;
		while(pPrevStat!=pCurStat){
			if(pPrevStat->bVisible)
				SubtractClipRegion(pCurStat->pClipRgn,&(pPrevStat->rc));
			pPrevStat=pPrevStat->pNext;
		}
		pCurStat=pCurStat->pNext;
	}
	if(IsVisible(_lGUI_pImeWindow)){
		pCurStat=pStat;
		while(pCurStat){
			SubtractClipRegion(pCurStat->pClipRgn,&(_lGUI_pImeWindow->rect));
			pCurStat=pCurStat->pNext;
		}
	}
	if(IsVisible(_lGUI_pSkbWindow)){
		pCurStat=pStat;
		while(pCurStat){
			SubtractClipRegion(pCurStat->pClipRgn,&(_lGUI_pSkbWindow->rect));
			pCurStat=pCurStat->pNext;
		}
	}

}


//===============================================================================
//****************************client*********************************************
//===============================================================================

//Initial ipc socket for client
BOOL 
InitIpcSocketClient()
{
	_lGUI_fdSocket=cli_conn(CS_PATH);
	if(_lGUI_fdSocket<0)
		return false;
	pthread_create(&thread_ipcmsg,NULL,ReadMainLoopClient,(void*)&_lGUI_fdSocket);
	return true;
}

//thread main loop for client socket
void* 
ReadMainLoopClient(
	void* srvfd
)
{
	void* pData;
	int iSize;
	int iRet;
	int old_cancel_type;
	int fd;
	int iMsg;
	fd=*((int*)srvfd);

	pData=malloc(SIZE_IPC_MSG);
	if(!pData)
		pthread_exit(0);
	//register thread cleanup function
	pthread_cleanup_push(cleanup_after_malloc,pData);
	pthread_cleanup_push(cleanup_after_opensock,srvfd);
	while(1){
		iRet = sock_read(fd,(void*)&iMsg,sizeof(int));
		if(iRet==SOCKERR_CLOSED || iRet==SOCKERR_IO){
			//printerror("socket error while reading type of message!");
			pthread_exit(0);
		}
		//read the size of added data 
		iRet=sock_read(fd,(void*)&iSize,sizeof(int));
		if(iRet==SOCKERR_CLOSED || iRet==SOCKERR_IO){
			//printerror("socket error while reading size of message!");
			pthread_exit(0);
		}
		//read added data
		iRet=sock_read(fd,pData,iSize);
		if(iRet==SOCKERR_CLOSED || iRet==SOCKERR_IO){
			//printerror("socket error while reading message!");
			pthread_exit(0);
		}
		ProcessIpcMessageClient(iMsg,pData,iSize);
	}
	//thread cleanup function
	pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED,&old_cancel_type);
	pthread_cleanup_pop(1);
	pthread_cleanup_pop(1);
	pthread_setcanceltype(old_cancel_type,NULL);
}


//process ipc message for client
BOOL 
ProcessIpcMessageClient(
	int iMsg,  
	void* pAttachData, 
	int iAttachSize
)
{
	POINT point;
	PWindowsTree pControl,pChild;
	int iCounter,i;
	RECT rect;
	void* pCurPointer;
	int iMenuId;
	int iRetMsg;
	WPARAM wParam;
	
	PClipRect pCRect;
	switch (iMsg & MTYPE_MASK){
	case MTYPE_IPC:
		switch(iMsg){
		case LMSG_IPC_EXITSERVER:
			//server send the message to client when exit 
			DestroyWindow((HWND)_lGUI_pWindowsTree);
			break;
		case LMSG_IPC_SHOWMAINWIN_ANS:
			//confirm message for show main window 
			//together with initial clip region 
			pCurPointer=(void*)pAttachData;
			EmptyClipRegion(_lGUI_pAppStat->pClipRgn);
			iCounter=iAttachSize/sizeof(RECT);
			for(i=0;i<iCounter;i++){
				memcpy(&rect,pCurPointer,sizeof(RECT));
				AddRectClipRegion(_lGUI_pAppStat->pClipRgn, &rect);
				pCurPointer+=sizeof(RECT);
			}
			GetBoundClipRegion(_lGUI_pAppStat->pClipRgn);
			UnLockMutexForSynchro();
			break;
		case LMSG_IPC_CLIPRGNCHANGE:
			//clip region changed 
			pCurPointer=(void*)pAttachData;
			EmptyClipRegion(_lGUI_pAppStat->pClipRgn);
			iCounter=iAttachSize/sizeof(RECT);
			for(i=0;i<iCounter;i++){
				memcpy(&rect,pCurPointer,sizeof(RECT));

				AddRectClipRegion(_lGUI_pAppStat->pClipRgn, &rect);
				pCurPointer+=sizeof(RECT);
			}
			GetBoundClipRegion(_lGUI_pAppStat->pClipRgn);
			ReCalClipRegionApp();
			break;
		case LMSG_IPC_REDRAW:
			//redraw message
			memcpy(&rect,pAttachData,sizeof(RECT));//pAttachData:RECT

			scrInvalidateRect((HWND)_lGUI_pWindowsTree,&rect,true);
			//scrInvalidateRect((HWND)_lGUI_pWindowsTree,NULL,true);
			//redraw sub window
			pChild=_lGUI_pWindowsTree->pChildHead;
			while(pChild){
				scrInvalidateRect((HWND)pChild,&rect,true);
				pControl=pChild->pControlHead;
				while(pControl){
					scrInvalidateRect((HWND)pControl,&rect,true);
					//scrInvalidateRect((HWND)pControl,NULL,true);
					pControl=pControl->pNext;
				}
				pChild=pChild->pNext;
			}
			//redraw control
			pControl=_lGUI_pWindowsTree->pControlHead;
			while(pControl){
				scrInvalidateRect((HWND)pControl,&rect,true);
				//scrInvalidateRect((HWND)pControl,NULL,true);
				pControl=pControl->pNext;
			}
			break;
		case LMSG_IPC_ACTIVEAPP:
			ActiveApplication();
			break;
		case LMSG_IPC_DISACTIVEAPP:
			DisactiveApplication();
			break;
		case LMSG_IPC_CHAR:
			if(!_lGUI_pFocus)
				return false;
			else{
				//printf("%s,%d\n",(char*)pAttachData,iAttachSize);
				*((char*)pAttachData+iAttachSize)=0;
				SendMessage(_lGUI_pFocus,LMSG_CHAR,*((WPARAM*)pAttachData),(LPARAM)NULL);
			}	
			break;
		case LMSG_IPC_KEYDOWN:
			if(!_lGUI_pFocus)
				return false;
			else{
				wParam = (WPARAM)*((char*)pAttachData);
				SendMessage(_lGUI_pFocus,LMSG_KEYDOWN,wParam,(LPARAM)NULL);
			}
			break;

		}
		break;

/*	case MTYPE_KEY://key board 
		//iSendMsg=(iMsg & ~MTYPE_MASK) | MTYPE_KEY;
		//printf("%s\n",(char*)pAttachData);
		if(!_lGUI_pFocus)
			return false;
		else
			PostMessage(_lGUI_pFocus,iMsg,(WPARAM)pAttachData,(LPARAM)NULL);
		break;
*/
	case MTYPE_MOUSE://Mouse message 
		memcpy(&point,pAttachData,sizeof(POINT));
		return ProcessMouseMsgClient(iMsg,point.x,point.y);
	}
	return true;
}


//send message by client
BOOL 
SendMsgByClient(
	int iMsg,
	void* pAttachData,
	int iAttachSize
)
{
	void* pData;
	if(!_lGUI_fdSocket)
		return false;
	//send message value 
	sock_write(_lGUI_fdSocket,(void*)&iMsg,sizeof(int));
	//send size of added data
	sock_write(_lGUI_fdSocket,(void*)&iAttachSize,sizeof(int));
	//send added data
	if(pData && iAttachSize)
		sock_write(_lGUI_fdSocket,pAttachData,iAttachSize);
	return true;
}


BOOL 
ProcessMouseMsgClient(
	int iMsg,
	int x, 
	int y
)
{
	PClipRect pCRect;
	RECT rcClient;
	PWindowsTree pWin,pControl;
	switch(iMsg){
	case LMSG_PENDOWN:
		ProcessMouseDownMsgClient(iMsg,x,y);
		break;
	case LMSG_PENMOVE:
		ProcessMouseMoveMsgClient(iMsg,x,y);
		break;
	case LMSG_PENUP:
		ProcessMouseUpMsgClient(iMsg,x,y);
		break;
	}
	return true;
}


BOOL 
ProcessMouseDownMsgClient(
	int iMsg, 
	int x, 
	int y
)
{
	PClipRect pCRect;
	RECT rcClient;
	PWindowsTree pWin,pControl;
	//1. within sub window
	pWin=_lGUI_pWindowsTree->pChildHead;
	while(pWin){
		//1.1within clip region of the sub window
		if(PtInRect(&(pWin->rect),x,y)){
			pCRect=pWin->pClipRgn->pHead;
			while(pCRect){
				if(PtInRect(&(pCRect->rect),x,y)){
					//Move the sub window top. 
					if(pWin->pParent->pChildHead!=pWin)
						ShowWindow((HWND)pWin,true);
					else{
						ActiveWindow((HWND)pWin);
					}
					scrGetClientRect((HWND)pWin,&rcClient);
					if(PtInRect(&rcClient,x,y))
						PostMessage(pWin,LMSG_PENDOWN,(WPARAM)x,(LPARAM)y);
					else
						PostMessage(pWin,LMSG_NCPENDOWN,(WPARAM)x,(LPARAM)y);
					return true;
				}
				pCRect=pCRect->pNext;
			}
			//1.2within the control of the sub window
			pControl=pWin->pControlHead;
			while(pControl){
				if(PtInRect(&(pControl->rect),x,y)){
					//move the sub window top 
					if(pWin->pParent->pChildHead!=pWin)
						ShowWindow((HWND)pControl,true);
					scrGetClientRect((HWND)pControl,&rcClient);
					if(PtInRect(&rcClient,x,y))
						PostMessage(pControl,LMSG_PENDOWN,(WPARAM)x,(LPARAM)y);
					else
						PostMessage(pControl,LMSG_NCPENDOWN,(WPARAM)x,(LPARAM)y);
					SetFocus((HWND)pControl);
					return true;
				}
				pControl=pControl->pNext;
			}
		}
		pWin=pWin->pNext;
	}
	//2.within the main window
	pCRect=_lGUI_pWindowsTree->pClipRgn->pHead;
	while(pCRect){
		if(PtInRect(&(pCRect->rect),x,y)){
			scrGetClientRect((HWND)_lGUI_pWindowsTree,&rcClient);
			if(PtInRect(&rcClient,x,y)){
				PostMessage(_lGUI_pWindowsTree,LMSG_PENDOWN,(WPARAM)x,(LPARAM)y);
			}
			else{
				PostMessage(_lGUI_pWindowsTree,LMSG_NCPENDOWN,(WPARAM)x,(LPARAM)y);

			}
			if(!IsActive(_lGUI_pWindowsTree))
				ActiveWindow(_lGUI_pWindowsTree);
			return true;
		}
		pCRect=pCRect->pNext;
	}
	//3.within the control of the main window 
	pWin=_lGUI_pWindowsTree->pControlHead;
	while(pWin){
		pCRect=pWin->pClipRgn->pHead;
		while(pCRect){
			if(PtInRect(&(pCRect->rect),x,y)){
				scrGetClientRect((HWND)pWin,&rcClient);
				if(PtInRect(&rcClient,x,y))
					PostMessage(pWin,LMSG_PENDOWN,(WPARAM)x,(LPARAM)y);
				else
					PostMessage(pWin,LMSG_NCPENDOWN,(WPARAM)x,(LPARAM)y);
				if(!IsActive((HWND)_lGUI_pWindowsTree)){
					RECT rc;
					_lGUI_pActiveWin = _lGUI_pWindowsTree;
					_lGUI_pWindowsTree->dwStyle |=  WS_ACTIVE;
					wndGetCaptionRect((HWND)_lGUI_pWindowsTree,&rc);
					scrInvalidateRect((HWND)_lGUI_pWindowsTree,&rc,true);				
				}
				SetFocus(pWin);
				return true;
			}
			pCRect=pCRect->pNext;
		}
		pWin=pWin->pNext;
	}
	return true;

}


BOOL 
ProcessMouseMoveMsgClient(
	int iMsg, 
	int x, 
	int y
)
{
	PClipRect pCRect;
	RECT rcClient;
	PWindowsTree pWin,pControl;

	//who captured the mouse 
	if(_lGUI_wndCaptureMouse.pWin){
		//if within the clip region of the window/control which captured the mouse 
		//then send message to it
		if(PtInRect(&(_lGUI_wndCaptureMouse.pWin->pClipRgn->rcBound),x,y)){
			pCRect=_lGUI_wndCaptureMouse.pWin->pClipRgn->pHead;
			while(pCRect){
				if(PtInRect(&(pCRect->rect),x,y)){
					scrGetClientRect((HWND)(_lGUI_wndCaptureMouse.pWin),&rcClient);
					if((_lGUI_wndCaptureMouse.iByWho == BYCLIENT) && PtInRect(&rcClient,x,y)){
						PostMessage(_lGUI_wndCaptureMouse.pWin,LMSG_PENMOVE,(WPARAM)x,(LPARAM)y);
						return true;
					}
					if((_lGUI_wndCaptureMouse.iByWho == BYVSCROLL) ||(_lGUI_wndCaptureMouse.iByWho == BYHSCROLL)){
						PostMessage(_lGUI_wndCaptureMouse.pWin,LMSG_NCPENMOVE,(WPARAM)x,(LPARAM)y);
						return true;
					}
				}
				pCRect=pCRect->pNext;
			}
		}
	}
	else{//if no window/control captured mouse
		//then send the message to window/control which mouse is being over it
		//1.within a sub window
		pWin=_lGUI_pWindowsTree->pChildHead;
		while(pWin){
			if(PtInRect(&(pWin->rect),x,y)){
				//1.1 within the clip region of the sub window
				pCRect=pWin->pClipRgn->pHead;
				while(pCRect){
					if(PtInRect(&(pCRect->rect),x,y)){
						scrGetClientRect((HWND)pWin,&rcClient);
						if(PtInRect(&rcClient,x,y))
							PostMessage(pWin,LMSG_PENMOVE,(WPARAM)x,(LPARAM)y);
						else
							PostMessage(pWin,LMSG_NCPENMOVE,(WPARAM)x,(LPARAM)y);
						return true;
					}
					pCRect=pCRect->pNext;
				}
				//1.2 within controls of the sub window
				pControl=pWin->pControlHead;
				while(pControl){
					if(PtInRect(&(pControl->rect),x,y)){
						scrGetClientRect((HWND)pControl,&rcClient);
						if(PtInRect(&rcClient,x,y))
							PostMessage(pControl,LMSG_PENMOVE,(WPARAM)x,(LPARAM)y);
						else
							PostMessage(pControl,LMSG_NCPENMOVE,(WPARAM)x,(LPARAM)y);
						return true;
					}
					pControl=pControl->pNext;
				}
			}
			pWin=pWin->pNext;
		}
		//2.within clip region of main window 
		pCRect=_lGUI_pWindowsTree->pClipRgn->pHead;
		while(pCRect){
			if(PtInRect(&(pCRect->rect),x,y)){
					scrGetClientRect((HWND)_lGUI_pWindowsTree,&rcClient);
					if(PtInRect(&rcClient,x,y))
						PostMessage(_lGUI_pWindowsTree,LMSG_PENMOVE,(WPARAM)x,(LPARAM)y);
					else
						PostMessage(_lGUI_pWindowsTree,LMSG_NCPENMOVE,(WPARAM)x,(LPARAM)y);
				return true;
			}
			pCRect=pCRect->pNext;
		}
		//3.within clip region of controls of main window
		pWin=_lGUI_pWindowsTree->pControlHead;
		while(pWin){
			pCRect=pWin->pClipRgn->pHead;
			while(pCRect){
				if(PtInRect(&(pCRect->rect),x,y)){
					scrGetClientRect((HWND)pWin,&rcClient);
					if(PtInRect(&rcClient,x,y))
						PostMessage(pWin,LMSG_PENMOVE,(WPARAM)x,(LPARAM)y);
					else
						PostMessage(pWin,LMSG_NCPENMOVE,(WPARAM)x,(LPARAM)y);

					return true;
				}
				pCRect=pCRect->pNext;
			}
			pWin=pWin->pNext;
		}
	}

	return true;

}


BOOL 
ProcessMouseUpMsgClient(
	int iMsg, 
	int x,
	int y
)
{
	PClipRect pCRect;
	RECT rcClient;
	PWindowsTree pWin,pControl;


	if(_lGUI_wndCaptureMouse.pWin){
		//send the message to which captured mouse before.
		if(_lGUI_wndCaptureMouse.iByWho == BYCLIENT){
			PostMessage((HWND)_lGUI_wndCaptureMouse.pWin,LMSG_PENUP,(WPARAM)x,(LPARAM)y);
		}
		else{
			PostMessage((HWND)_lGUI_wndCaptureMouse.pWin,LMSG_NCPENUP,(WPARAM)x,(LPARAM)y);
		}

	}
	else{
		//if no window/control captured mouse before
		//then send the message to the control/window where mouse put up.
		//1.within a sub window
		pWin=_lGUI_pWindowsTree->pChildHead;
		while(pWin){
			if(PtInRect(&(pWin->rect),x,y)){
				//1.1 within clip region of the sub window
				pCRect=pWin->pClipRgn->pHead;
				while(pCRect){
					if(PtInRect(&(pCRect->rect),x,y)){
						scrGetClientRect((HWND)pWin,&rcClient);
						if(PtInRect(&rcClient,x,y))
							PostMessage(pWin,LMSG_PENUP,(WPARAM)x,(LPARAM)y);
						else
							PostMessage(pWin,LMSG_NCPENUP,(WPARAM)x,(LPARAM)y);
						return true;
					}
					pCRect=pCRect->pNext;
				}
				//1.2 within controls of the sub window
				pControl=pWin->pControlHead;
				while(pControl){
					if(PtInRect(&(pControl->rect),x,y)){
						scrGetClientRect((HWND)pControl,&rcClient);
						if(PtInRect(&rcClient,x,y))
							PostMessage(pControl,LMSG_PENUP,(WPARAM)x,(LPARAM)y);
						else
							PostMessage(pControl,LMSG_NCPENUP,(WPARAM)x,(LPARAM)y);
						return true;
					}
					pControl=pControl->pNext;
				}
			}
			pWin=pWin->pNext;
		}
		//2.within clip region of main window
		pCRect=_lGUI_pWindowsTree->pClipRgn->pHead;
		while(pCRect){
			if(PtInRect(&(pCRect->rect),x,y)){
				scrGetClientRect((HWND)_lGUI_pWindowsTree,&rcClient);
				if(PtInRect(&rcClient,x,y))
					PostMessage(_lGUI_pWindowsTree,LMSG_PENUP,(WPARAM)x,(LPARAM)y);
				else
					PostMessage(_lGUI_pWindowsTree,LMSG_NCPENUP,(WPARAM)x,(LPARAM)y);
				return true;
			}
			pCRect=pCRect->pNext;
		}
		//3.within controls of main window
		pWin=_lGUI_pWindowsTree->pControlHead;
		while(pWin){
			pCRect=pWin->pClipRgn->pHead;
			while(pCRect){
				if(PtInRect(&(pCRect->rect),x,y)){
					scrGetClientRect((HWND)pWin,&rcClient);
					if(PtInRect(&rcClient,x,y))
						PostMessage(pWin,LMSG_PENUP,(WPARAM)x,(LPARAM)y);
					else
						PostMessage(pWin,LMSG_NCPENUP,(WPARAM)x,(LPARAM)y);
					return true;
				}
				pCRect=pCRect->pNext;
			}
			pWin=pWin->pNext;
		}
	}
	return true;
}



void 
SendString2Client(
	char* pString
)
{

	if(!_lGUI_pAppStat)
		return;
	SendMsgByServer(_lGUI_pAppStat,LMSG_IPC_CHAR,(void*)pString,strlen(pString));
}

void 
SendKeyDown2Client(
	BYTE byteKeyValue
)
{
	char pString[3];
	pString[0] = byteKeyValue;
	pString[1] = '\0';
	if(!_lGUI_pAppStat)
		return;
	SendMsgByServer(_lGUI_pAppStat,LMSG_IPC_KEYDOWN,(void*)pString,strlen(pString));
}


