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
#include "../include/filedir.h"

int 
GetFileList(
	char* lpszPath, 
	struct dirent ***pFileList
)
{
    return scandir(lpszPath, pFileList, FileSelect, alphasort);
}

int 
FileSelect(
	const struct dirent *pItem
)
{
	if((strcmp(pItem->d_name,".")!=0)&&(strcmp(pItem->d_name,"..")!=0))
		return 1;
	else
	    return 0;
}

int 
FileSort(
	const void *a, 
	const void *b
)
{
	struct stat bufa;
	struct stat bufb;
	stat( (*(struct dirent **)a)->d_name, &bufa);
	stat( (*(struct dirent **)b)->d_name, &bufb);
	if(bufa.st_mtime < bufb.st_mtime)
		return 1;
	else
		return -1;
}

void 
FreeFileList(
	struct dirent **pFileList, 
	int iFileNumber
)
{
	while(iFileNumber--) {
		free(pFileList[iFileNumber]);
	}
	free(pFileList);
}


