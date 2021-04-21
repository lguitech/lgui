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
#include "../include/mouse.h"
#include "../include/rect.h"
#include "../include/framebuffer.h"

extern pthread_mutex_t mutex_mouse;

static unsigned char* _lGUI_pFrameBuffer;

static struct fb_fix_screeninfo _lGUI_fInfo;
static struct fb_var_screeninfo _lGUI_vInfo;
static int _lGUI_iFrameBuffer;
static int _lGUI_iLineSize;

static int _lGUI_iFrameWidth;
static int _lGUI_iFrameHeight;

static int _lGUI_iBufferSize;

int _lGUI_iBytesPerPixel = 0;

int _lGUI_iBytesDataType = 0;

BOOL 
InitFrameBuffer()
{
    _lGUI_iFrameBuffer = open ("/dev/fb0", O_RDWR);
	if(!_lGUI_iFrameBuffer){
		printerror("open framebuffer return error.");
		return FALSE;
	}
	ioctl (_lGUI_iFrameBuffer, FBIOGET_VSCREENINFO, &_lGUI_vInfo);

	_lGUI_iFrameHeight=_lGUI_vInfo.yres;
	_lGUI_iFrameWidth=_lGUI_vInfo.xres;

	_lGUI_iLineSize = _lGUI_vInfo.xres * _lGUI_vInfo.bits_per_pixel / 8;
	_lGUI_iBufferSize = _lGUI_iLineSize * _lGUI_vInfo.yres;

	_lGUI_vInfo.xoffset = 0;
	_lGUI_vInfo.yoffset = 0;
	ioctl (_lGUI_iFrameBuffer, FBIOPAN_DISPLAY, &_lGUI_vInfo);
	_lGUI_pFrameBuffer =mmap (NULL, _lGUI_iBufferSize, PROT_READ | PROT_WRITE, MAP_SHARED, _lGUI_iFrameBuffer, 0);
	if(!_lGUI_pFrameBuffer){
		printerror("mmap return error.");
		return false;
	}
	_lGUI_iBytesPerPixel = _lGUI_vInfo.bits_per_pixel / 8;
	if (_lGUI_iBytesPerPixel == 3)
		_lGUI_iBytesDataType = 4;
	else
		_lGUI_iBytesDataType = _lGUI_iBytesPerPixel;
	
	return true;
}

void 
UnInitFrameBuffer()
{
	close(_lGUI_iFrameBuffer);
}

void inline 
lGUI_SetPixel_Direct(
	int x,
	int y, 
	COLORREF color
)
{
	unsigned char* pDest;
	pDest = _lGUI_pFrameBuffer + _lGUI_iLineSize * y + (x * _lGUI_iBytesPerPixel);

	if(_lGUI_iBytesPerPixel == 3){
		*pDest = B(color);
		*(pDest+1) = G(color);
		*(pDest+2) = R(color);
	}
	else	
		*((PCOLORREF)pDest)=color;

}

void inline 
lGUI_SetPixel(
	int x,
	int y, 
	COLORREF color
)
{
	unsigned char* pDest;
	RECT rc;
	SetRect(&rc,x,y,x,y);
	RequestPaint(&rc);

	//printf("%d,%d\n",x,y);
	//pDest = _lGUI_pFrameBuffer + _lGUI_iLineSize * y + (x << (_lGUI_iBytesPerPixel-1));
	pDest = _lGUI_pFrameBuffer + _lGUI_iLineSize * y + (x * _lGUI_iBytesPerPixel);
	if(_lGUI_iBytesPerPixel == 3){
		*pDest = B(color);
		*(pDest+1) = G(color);
		*(pDest+2) = R(color);
	}
	else	
		*((PCOLORREF)pDest)=color;

	CompletePaint();
}

COLORREF inline 
lGUI_GetPixel(
	int x,
	int y
)
{
	COLORREF crColor;
	unsigned char* pDest;
	RECT rc;
	SetRect(&rc,x,y,x,y);
	RequestPaint(&rc);

	pDest = _lGUI_pFrameBuffer + _lGUI_iLineSize * y + (x * _lGUI_iBytesPerPixel);

	if(_lGUI_iBytesPerPixel == 3)
		crColor = 0xff000000 | RGB(*(pDest+2), *(pDest+1), *pDest);
	else
		crColor = *((PCOLORREF)pDest);
	
		
	CompletePaint();

	return crColor;
}

COLORREF inline 
lGUI_GetPixel_Direct(
	int x,
	int y
)
{
	unsigned char* pDest;
	COLORREF crColor;
	pDest = _lGUI_pFrameBuffer + _lGUI_iLineSize * y + (x * _lGUI_iBytesPerPixel);

	if(_lGUI_iBytesPerPixel == 3)
		crColor = 0xff000000 | RGB(*(pDest+2), *(pDest+1), *pDest);
	else
		crColor = *((PCOLORREF)pDest);
	
	return crColor;
}



void 
PaintRect2Screen(
	PRECT lpOutRc,
	PRECT lpOrgRc,
	void* pData
)
{

	int x,y;
	unsigned char* pCurPointer;
	unsigned char* pDest;
	int iOrgWidth;
	int iSourceLeftMarge,iTargetLeftMarge;

	RequestPaint(lpOutRc);


	iOrgWidth=(lpOrgRc->right-lpOrgRc->left+1)* _lGUI_iBytesDataType;
	iSourceLeftMarge=(lpOutRc->left-lpOrgRc->left)*_lGUI_iBytesDataType;
	iTargetLeftMarge=lpOutRc->left*_lGUI_iBytesPerPixel;

	if(_lGUI_iBytesPerPixel == 3){
		for(y = lpOutRc->top; y <= lpOutRc->bottom; y++){
			pCurPointer=pData + iOrgWidth*(y-lpOrgRc->top) + iSourceLeftMarge;
			pDest=_lGUI_pFrameBuffer + _lGUI_iLineSize*y + iTargetLeftMarge;

			for(x = lpOutRc->left; x <= lpOutRc->right; x++){
				*pDest = B(*pCurPointer);
				*(pDest+1) = G(*pCurPointer);
				*(pDest+2) = R(*pCurPointer);
				pDest +=3;
				pCurPointer += _lGUI_iBytesDataType;
			}
		}
	}
	else{
		for(y=lpOutRc->top;y<=lpOutRc->bottom;y++){
			pCurPointer=pData + iOrgWidth*(y-lpOrgRc->top) + iSourceLeftMarge;
			pDest=_lGUI_pFrameBuffer + _lGUI_iLineSize*y + iTargetLeftMarge;
			memcpy(pDest,pCurPointer,(lpOutRc->right-lpOutRc->left+1)*_lGUI_iBytesPerPixel);
		}
	}

	CompletePaint();

}

void 
PaintRect2ScreenWithTrColor(
	PRECT lpOutRc,
	PRECT lpOrgRc,
	void* pData, 
	COLORREF crTrColor
)
{
	int x,y;
	unsigned char* pCurPointer;
	unsigned char* pDest;
	int iOrgWidth;



	RequestPaint(lpOutRc);

	iOrgWidth=(lpOrgRc->right-lpOrgRc->left+1)* _lGUI_iBytesDataType;

	if(_lGUI_iBytesPerPixel == 3){
		for(y=lpOutRc->top;y<=lpOutRc->bottom;y++){
			for(x=lpOutRc->left;x<=lpOutRc->right;x++){
				pCurPointer=pData + iOrgWidth*(y-lpOrgRc->top)+(x-lpOrgRc->left)*_lGUI_iBytesDataType;
				if(*((PCOLORREF)pCurPointer)!=crTrColor){
					
					pDest = _lGUI_pFrameBuffer + _lGUI_iLineSize*y + x* _lGUI_iBytesPerPixel;
					
					*pDest = B(*pCurPointer);
					*(pDest+1) = G(*pCurPointer);
					*(pDest+2) = R(*pCurPointer);
					
				}
			}
		}

	}
	else{
		for(y=lpOutRc->top;y<=lpOutRc->bottom;y++){
			for(x=lpOutRc->left;x<=lpOutRc->right;x++){
				pCurPointer=pData + iOrgWidth*(y-lpOrgRc->top)+(x-lpOrgRc->left)*_lGUI_iBytesDataType;
				if(*((PCOLORREF)pCurPointer)!=crTrColor){
					pDest=_lGUI_pFrameBuffer + _lGUI_iLineSize*y + x* _lGUI_iBytesDataType;
					memcpy(pDest,pCurPointer,_lGUI_iBytesDataType);
				}
			}
		}

	}
	
	CompletePaint();
}



void 
PaintLine2Screen(
	int xPos, 
	int yPos, 
	int iLen, 
	void* pData
)
{
	unsigned char* pDest;

	RECT rcOut;
	int i;

	PCOLORREF pSource;

	SetRect(&rcOut,xPos,yPos, xPos+iLen,yPos);
	
	RequestPaint(&rcOut);

	pSource = (PCOLORREF)pData;

	pDest=_lGUI_pFrameBuffer + _lGUI_iLineSize*yPos + xPos * _lGUI_iBytesPerPixel;
	if(_lGUI_iBytesPerPixel == 3)
		for(i=0;i<iLen;i++){
			*pDest = B(*pSource);
			*(pDest+1) = G(*pSource);
			*(pDest+2) = R(*pSource);

			pDest +=3;
			pSource += 1;
			
		}
	else
		memcpy(pDest,pData,iLen*_lGUI_iBytesPerPixel);

	CompletePaint();

}


void inline 
PaintLine2ScreenRAW(
	int xPos,
	int yPos, 
	int iLen, 
	void* pData
)
{

	int i;
	unsigned char* pDest;
	PCOLORREF pSource;

	pSource = (PCOLORREF)pData;

	pDest=_lGUI_pFrameBuffer + _lGUI_iLineSize*yPos + xPos * _lGUI_iBytesPerPixel;
	if(_lGUI_iBytesPerPixel == 3)
		for(i=0;i<iLen;i++){
			*pDest = B(*pSource);
			*(pDest+1) = G(*pSource);
			*(pDest+2) = R(*pSource);

			pDest +=3;
			pSource += 1;
			
		}
	else
		memcpy(pDest,pData,iLen*_lGUI_iBytesPerPixel);

}


void 
GetLineFromScreen(
	int xPos, 
	int yPos, 
	int iLen, 
	void* pData
)
{
	unsigned char* pSrc;

	RECT rcOut;

	int i;
	PCOLORREF pTarget;
	
	SetRect(&rcOut,xPos,yPos, xPos+iLen,yPos);
	
	RequestPaint(&rcOut);
	pSrc=_lGUI_pFrameBuffer + _lGUI_iLineSize*yPos + xPos * _lGUI_iBytesPerPixel;
	
	if(_lGUI_iBytesPerPixel == 3){
		for(i=0;i<iLen;i++){
			*pTarget = 0xff000000 | RGB(*pSrc, *(pSrc+1),*(pSrc+2));
			pTarget +=1;
			pSrc += _lGUI_iBytesPerPixel;
		}
	}
	else
		memcpy(pData,pSrc,iLen*_lGUI_iBytesPerPixel);

	CompletePaint();

}

void 
GetLineFromScreenRAW(
	int xPos, 
	int yPos, 
	int iLen, 
	void* pData
)
{
	int i;
	unsigned char* pSrc;
	PCOLORREF pTarget;
	
	pSrc=_lGUI_pFrameBuffer + _lGUI_iLineSize*yPos + xPos * _lGUI_iBytesPerPixel;
	
	if(_lGUI_iBytesPerPixel == 3){
		for(i=0;i<iLen;i++){
			*pTarget = 0xff000000 | RGB(*(pSrc+2), *(pSrc+1),*pSrc);
			pTarget +=1;
			pSrc += _lGUI_iBytesPerPixel;
		}
	}
	else
		memcpy(pData,pSrc,iLen*_lGUI_iBytesPerPixel);
}


void 
EraseRect2Screen(
	PRECT lpRc,
	COLORREF color
)
{
	int x,y;
	int iLeftMarge;

	RequestPaint(lpRc);

	iLeftMarge=lpRc->left;
	for(y=lpRc->top;y<=lpRc->bottom;y++)
		for(x=lpRc->left;x<=lpRc->right;x++)
			lGUI_SetPixel_Direct(x,y,color);

	CompletePaint();

}



inline void 
GetBoxFromScreen(
	int x, 
	int y, 
	int nWidth, 
	int nHeight, 
	PCOLORREF pData
)
{
	int i,j;
	RECT rc;
	SetRect(&rc, x, y, x+nWidth, y+nHeight);
	RequestPaint(&rc);

	for(i=0; (i<nHeight)&&(y+i<SCREEN_HEIGHT); i++){
		for(j=0;(j<nWidth)&&(x+j<SCREEN_WIDTH); j++){
			*(pData + i*nWidth + j) = lGUI_GetPixel_Direct(x+j,y+i);
		}
	}

	CompletePaint();
}

inline void 
GetBoxFromScreenRaw(
	int x,
	int y, 
	int nWidth, 
	int nHeight, 
	PCOLORREF pData
)
{
	int i,j;
	for(i=0; (i<nHeight)&&(y+i<SCREEN_HEIGHT); i++){
		for(j=0;(j<nWidth)&&(x+j<SCREEN_WIDTH); j++){
			*(pData + i*nWidth + j) = lGUI_GetPixel_Direct(x+j,y+i);
		}
	}
}


inline void 
PutBox2Screen(
	int x, 
	int y, 
	int nWidth, 
	int nHeight, 
	PCOLORREF pData
)
{
	int i,j;
	for(i=0; (i<nHeight) && (y + i < SCREEN_HEIGHT); i++){
		for(j=0; (j<nWidth) && (x + j < SCREEN_WIDTH);j++){
			lGUI_SetPixel_Direct(x+j,y+i,*(pData + i*nWidth + j));
		}
	}
}

inline void 
PutBox2ScreenWithTrColor(
	int x, 
	int y, 
	int nWidth, 
	int nHeight, 
	PCOLORREF pData, 
	COLORREF crColor
)
{
	int i,j;
	COLORREF crPointColor;
	
	//printf("%d,%d\n",x,y);
	for(i=0; (i<nHeight) && (y + i < SCREEN_HEIGHT); i++){
		for(j=0; (j<nWidth) && (x + j < SCREEN_WIDTH);j++){
			crPointColor = *(pData + i*nWidth + j);
			if(crPointColor!=crColor){
				lGUI_SetPixel_Direct(x+j,y+i,crPointColor);
			}
		}
	}
}


