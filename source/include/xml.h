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

#ifndef _XML_H
#define _XML_H

#ifdef __cplusplus                     
extern "C" {
#endif

#define MAX_TAG_SIZE	16
#define MAX_VALUE_SIZE	256

BOOL 
GetATag(
	FILE* fp, 
	char* lpszRetTag
);


BOOL 
IsDirHead(
	char* lpszTag
);


BOOL 
IsDirTail(
	char* lpszTag
);


BOOL
IsAppHead(
	char* lpszTag
);


BOOL 
IsAppTail(
	char* lpszTag
);


BOOL 
GetHeadTag(
	FILE* fp, 
	char* lpszRetTag
);


BOOL 
GetTailTag(
	FILE* fp, 
	char* lpszRetTag
);


BOOL 
GetValue(
	FILE* fp, 
	char* lpszItem, 
	char* lpszValue
);

#ifdef __cplusplus
}
#endif 


#endif 
