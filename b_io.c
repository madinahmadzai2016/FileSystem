/**************************************************************
* Class:  CSC-415-01 Spring 2022
* Names: John Santiago, Muhammed Nafees, Janelle Lara, Madina Ahmadzai
* Student IDs: 909606963, 921941329, 920156598, 921835158
* GitHub Name: aktails
* Group Name: MJ's
* Project: Basic File System
*
* File: b_io.c
*
* Description: Basic File System - Key File I/O Operations
*
**************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>			// for malloc
#include <string.h>			// for memcpy
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "b_io.h"
#include "fsLow.h"
#include "mfs.h"
#include "fsInit.h"

#define MAXFCBS 20
#define B_CHUNK_SIZE 512

char copyFileName[11];

typedef struct b_fcb
	{
	/** TODO add al the information you need in the file control block **/
	char fileName[11];
	int flag;
	struct Directory * parentDirectory;
	int fileStartingBlock;
	int fileSize;
	int blockCount;
	char * buf;		//holds the open file buffer
	int index;		//holds the current position in the buffer
	int buflen;		//holds how many valid bytes are in the buffer
	} b_fcb;
	
b_fcb fcbArray[MAXFCBS];

int startup = 0;	//Indicates that this has not been initialized

//Method to initialize our file system
void b_init ()
	{
	//init fcbArray to all free
	for (int i = 0; i < MAXFCBS; i++)
		{
		fcbArray[i].buf = NULL; //indicates a free fcbArray
		}
		
	startup = 1;
	}

//Method to get a free FCB element
b_io_fd b_getFCB ()
	{
	for (int i = 0; i < MAXFCBS; i++)
		{
		if (fcbArray[i].buf == NULL)
			{
			return i;		//Not thread safe (But do not worry about it for this assignment)
			}
		}
	return (-1);  //all in use
	}
	
// Interface to open a buffered file
// Modification of interface for this assignment, flags match the Linux flags for open
// O_RDONLY, O_WRONLY, or O_RDWR
b_io_fd b_open (char * filename, int flags)
	{
	b_io_fd returnFd;
		
	if (startup == 0) b_init();  //Initialize our system
	
	returnFd = b_getFCB();				// get our own file descriptor
										// check for error - all used FCB's
	if(returnFd != -1)
		{ // checks whether file is to be read or written to
		if(flags == O_RDONLY)
			{
			if(isValidPath(filename, 0) == 0)
				{
				fcbArray[returnFd].parentDirectory = searchDir;
				fcbArray[returnFd].flag = O_RDONLY;
				for(int i = 2; i < 16; i++)
					{
					if(strcmp(fcbArray[returnFd].parentDirectory->Directory[i].DIR_Name, lastFileName) == 0)
						{ // get inportant file information
						fcbArray[returnFd].fileStartingBlock = getStartingBlock(i);
						fcbArray[returnFd].fileSize = searchDir->Directory[i].DIR_FileSize;
						fcbArray[returnFd].blockCount = fcbArray[returnFd].fileSize / B_CHUNK_SIZE;
						if(fcbArray[returnFd].fileSize % B_CHUNK_SIZE > 0)
							{
							fcbArray[returnFd].blockCount++;
							}
						fcbArray[returnFd].buf = malloc(fcbArray[returnFd].blockCount * B_CHUNK_SIZE);
						LBAread(fcbArray[returnFd].buf, fcbArray[returnFd].blockCount,
										fcbArray[returnFd].fileStartingBlock);
						fcbArray[returnFd].index = 0;
						fcbArray[returnFd].buflen = fcbArray[returnFd].fileSize;
						}
					}
				}
			}
		else
			{
			if(isValidPath(filename, 0) == 0)
				{
				fs_delete(filename);
				if(isValidPath(filename, 0) == 0)
					{/* no-op */}
				}
			// prepare file to be written
			strcpy(fcbArray[returnFd].fileName, copyFileName);
			copyFileName[0] = '\0';
			fcbArray[returnFd].parentDirectory = searchDir;
			fcbArray[returnFd].flag = O_WRONLY | O_CREAT | O_TRUNC;
			fcbArray[returnFd].fileStartingBlock = 0;
			fcbArray[returnFd].fileSize = 0;
			fcbArray[returnFd].blockCount = 1;
			fcbArray[returnFd].buf = malloc(B_CHUNK_SIZE);
			fcbArray[returnFd].index = 0;
			fcbArray[returnFd].buflen = 0;
			}
		}
	return returnFd;						// all set
	}


// Interface to seek function	
int b_seek (b_io_fd fd, off_t offset, int whence)
	{
	if (startup == 0) b_init();  //Initialize our system

	// check that fd is between 0 and (MAXFCBS-1)
	if ((fd < 0) || (fd >= MAXFCBS))
		{
		return (-1); 					//invalid file descriptor
		}
	int newOffset;

	switch (whence)
		{
		case SEEK_SET:
			{
			newOffset = offset;
			break;
			}
		case SEEK_CUR:
			{
			newOffset = fcbArray[fd].index + offset;
			break;
			}
		case SEEK_END:
			{
			newOffset = fcbArray[fd].fileSize + offset;
			break;
			}
		default:
			return -1;
		}
	
	return newOffset;
	}



// Interface to write function	
int b_write (b_io_fd fd, char * buffer, int count)
	{
	if (startup == 0) b_init();  //Initialize our system

	// check that fd is between 0 and (MAXFCBS-1)
	if ((fd < 0) || (fd >= MAXFCBS))
		{
		return (-1); 					//invalid file descriptor
		}
	if(count + fcbArray[fd].buflen > fcbArray[fd].blockCount * B_CHUNK_SIZE)
		{
		fcbArray[fd].blockCount++;
		fcbArray[fd].buf = realloc(fcbArray[fd].buf, fcbArray[fd].blockCount * B_CHUNK_SIZE);
		}
	memcpy(fcbArray[fd].buf + fcbArray[fd].index, buffer, count);
	fcbArray[fd].index = b_seek(fd, count, SEEK_CUR);
	fcbArray[fd].buflen += count;

	return count; //Change this
	}



// Interface to read a buffer

// Filling the callers request is broken into three parts
// Part 1 is what can be filled from the current buffer, which may or may not be enough
// Part 2 is after using what was left in our buffer there is still 1 or more block
//        size chunks needed to fill the callers request.  This represents the number of
//        bytes in multiples of the blocksize.
// Part 3 is a value less than blocksize which is what remains to copy to the callers buffer
//        after fulfilling part 1 and part 2.  This would always be filled from a refill 
//        of our buffer.
//  +-------------+------------------------------------------------+--------+
//  |             |                                                |        |
//  | filled from |  filled direct in multiples of the block size  | filled |
//  | existing    |                                                | from   |
//  | buffer      |                                                |refilled|
//  |             |                                                | buffer |
//  |             |                                                |        |
//  | Part1       |  Part 2                                        | Part3  |
//  +-------------+------------------------------------------------+--------+
int b_read (b_io_fd fd, char * buffer, int count)
	{

	if (startup == 0) b_init();  //Initialize our system

	// check that fd is between 0 and (MAXFCBS-1)
	if ((fd < 0) || (fd >= MAXFCBS))
		{
		return (-1); 					//invalid file descriptor
		}
	int bytesRead = fcbArray[fd].buflen;
	if(fcbArray[fd].buflen > count)
		{
		bytesRead = count;
		}
	memcpy(buffer, fcbArray[fd].buf + fcbArray[fd].index, bytesRead);
	fcbArray[fd].index = b_seek(fd, bytesRead, SEEK_CUR);
	fcbArray[fd].buflen -= bytesRead;
		
	return bytesRead;	//Change this
	}
	
// Interface to Close the file	
void b_close (b_io_fd fd)
	{
	if(fcbArray[fd].flag != O_RDONLY)
		{
		fcbArray[fd].fileSize = fcbArray[fd].buflen;
		fcbArray[fd].fileStartingBlock = allocateBlocks(fcbArray[fd].fileSize);
		for(int i = 2; i < 16; i++)
			{
			if(fcbArray[fd].parentDirectory->Directory[i].DIR_Name[0] == 0x00)
				{
				strcpy(fcbArray[fd].parentDirectory->Directory[i].DIR_Name, fcbArray[fd].fileName);
				fcbArray[fd].parentDirectory->Directory[i].DIR_Attr = 0;
				fcbArray[fd].parentDirectory->Directory[i].DIR_NTRes = 0;
				fcbArray[fd].parentDirectory->Directory[i].DIR_CrtTimeTenth = 0;
				fcbArray[fd].parentDirectory->Directory[i].DIR_CrtTime = 0;
				fcbArray[fd].parentDirectory->Directory[i].DIR_CrtDate = 0;
				fcbArray[fd].parentDirectory->Directory[i].DIR_LstAccDate = 0;
				fcbArray[fd].parentDirectory->Directory[i].DIR_FstClusHI = fcbArray[fd].
																																	 fileStartingBlock >> 8;
				fcbArray[fd].parentDirectory->Directory[i].DIR_FstClusLO = fcbArray[fd].
																																	 fileStartingBlock % 256;
				fcbArray[fd].parentDirectory->Directory[i].DIR_FileSize = fcbArray[fd].fileSize;
				fcbArray[fd].parentDirectory->Directory[i].DIR_WrtTime = getCurrentTime();
				fcbArray[fd].parentDirectory->Directory[i].DIR_WrtDate = getCurrentDate();
				break;
				}
			}
		LBAwrite(fcbArray[fd].buf, fcbArray[fd].blockCount, fcbArray[fd].fileStartingBlock);
		int parentStartingBlock = fcbArray[fd].parentDirectory->Directory[0].DIR_FstClusHI << 8 |
															fcbArray[fd].parentDirectory->Directory[0].DIR_FstClusLO;
		LBAwrite(fcbArray[fd].parentDirectory, 1, parentStartingBlock);
		}
	fcbArray[fd].flag = 0;
	fcbArray[fd].fileStartingBlock = 0;
	fcbArray[fd].fileSize = 0;
	//free(fcbArray[fd].buf);
	fcbArray[fd].buf = NULL;
	fcbArray[fd].buflen = 0;
	fcbArray[fd].index = 0;
	}