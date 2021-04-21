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

#ifndef _SOCKIO_H_
#define _SOCKIO_H_

#ifdef __cplusplus                     
extern "C" {
#endif

#define CS_PATH		"/var/tmp/lgui"
#define CLI_PATH	"/var/tmp/"

#define SOCKERR_IO          -1
#define SOCKERR_CLOSED      -2
#define SOCKERR_INVARG      -3
#define SOCKERR_OK          0

int 
serv_listen(
	const char *name
);

int 
serv_accept(
	int listenfd
);

int 
cli_conn(
	const char *name
);

int 
sock_write(
	int fd, 
	const void* buff, 
	int count
);

int 
sock_read(
	int fd,
	void* buff, 
	int count
);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _SOCKIO_H_ */

 
