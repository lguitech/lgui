/*
	Copyright (C) 2004-2005 Li Yudong
	The idea comes from MiniGUI
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


BOOL 
HeapCreate(
	PPrivateHeap pHeap,
	unsigned long BlockNumber,
	unsigned long BlockSize
)
{
	pthread_mutex_init(&pHeap->mutex,NULL);
	pHeap->BlockNumber = BlockNumber;
	pHeap->BlockSize = BlockSize + sizeof(unsigned long);
	pHeap->free = 0;
	pHeap->pData = calloc(pHeap->BlockNumber,pHeap->BlockSize);
	if(!pHeap->pData)
		return false;
	memset(pHeap->pData,0,pHeap->BlockNumber*pHeap->BlockSize);//maybe unnecessary
	return true;

}

void*
HeapAlloc(
	PPrivateHeap pHeap
)
{
	char* pData=NULL;
	unsigned long i;
	pthread_mutex_lock(&pHeap->mutex);

	pData = pHeap->pData;
	for(i=pHeap->free;i<pHeap->BlockNumber;i++){
		if(*((unsigned long*)pData) == HEAP_BLOCK_FREE){
			*((unsigned long*)pData) = HEAP_BLOCK_USED;
			pHeap->free = i + 1;
			pthread_mutex_unlock(&pHeap->mutex);
			return pData + sizeof(unsigned long);
		}
		pData += pHeap->BlockSize;
	}
	
	pData = calloc(1,pHeap->BlockSize);
	*((unsigned long*)pData) = HEAP_BLOCK_OVERFLOW;

	pData += sizeof(unsigned long);

	pthread_mutex_unlock(&pHeap->mutex);
	return pData;
}

void 
HeapFree(
	PPrivateHeap pHeap,
	void* pData
)
{
	int i;
	char* pBlock;
	pthread_mutex_lock(&pHeap->mutex);
	pBlock = pData - sizeof(unsigned long);
	if(*((unsigned long*)pBlock) == HEAP_BLOCK_OVERFLOW){
		free(pBlock);
	}
	else{
		*((unsigned long*)pBlock) = HEAP_BLOCK_FREE;
		i = (pBlock - (char*)pHeap->pData)/pHeap->BlockSize;
		if(pHeap->free > i)
			pHeap->free = i;
	}
	pthread_mutex_unlock(&pHeap->mutex);
}





void HeapDestroy(PPrivateHeap pHeap)
{
	free(pHeap->pData);
}


