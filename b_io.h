/**************************************************************
* Class:  CSC-415-01 Spring 2022
* Names: John Santiago, Muhammed Nafees, Janelle Lara, Madina Ahmadzai
* Student IDs: 909606963, 921941329, 920156598, 921835158
* GitHub Name: aktails
* Group Name: MJ's
* Project: Basic File System
*
* File: b_io.h
*
* Description: Interface of basic I/O functions
*
**************************************************************/

#ifndef _B_IO_H
#define _B_IO_H
#include <fcntl.h>

typedef int b_io_fd;

void b_init ();
b_io_fd b_getFCB ();
b_io_fd b_open (char * filename, int flags);
int b_read (b_io_fd fd, char * buffer, int count);
int b_write (b_io_fd fd, char * buffer, int count);
int b_seek (b_io_fd fd, off_t offset, int whence);
void b_close (b_io_fd fd);

extern char copyFileName[11];

#endif

