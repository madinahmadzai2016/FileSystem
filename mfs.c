/**************************************************************
* Class:  CSC-415-01 Spring 2022
* Names: John Santiago, Muhammed Nafees, Janelle Lara, Madina Ahmadzai
* Student IDs: 909606963, 921941329, 920156598, 921835158
* GitHub Name: aktails
* Group Name: MJ's
* Project: Basic File System
*
* File: mfs.c
*
* Description: Main file system functions needed by the driver
* to interact with our our file system.
*
**************************************************************/

#include <string.h>
#include "mfs.h"
#include "fsInit.h"
#include "fsLow.h"

#define DIRMAX_LEN		4096

char cwd[DIRMAX_LEN] = "";
char lastFileName[11];
int readingDirectory = 0;


int fs_mkdir(const char *pathname, mode_t mode)
  {
  if(isValidPath(pathname, 1) == 0)
    {
    if(isValidPath(pathname, 0) != 0)
      {
        for(int i = 2; i < 16; i++)
        {
        if(searchDir->Directory[i].DIR_Name[0] == 0x00)
          {
          int newDirStartingBlock = allocateBlocks(VCBptr->BytesPerSector);
          if(newDirStartingBlock > 0)
            {
            // update parent directory
            strcpy(searchDir->Directory[i].DIR_Name, lastFileName);
            searchDir->Directory[i].DIR_Attr = 16;
            searchDir->Directory[i].DIR_FstClusHI = newDirStartingBlock >> 8;
            searchDir->Directory[i].DIR_FstClusLO = newDirStartingBlock % 256;
            searchDir->Directory[i].DIR_FileSize = VCBptr->BytesPerSector;
            searchDir->Directory[i].DIR_WrtTime = getCurrentTime();
            searchDir->Directory[i].DIR_WrtDate = getCurrentDate();
            searchDir->Directory[0].DIR_WrtTime = getCurrentTime();
            searchDir->Directory[0].DIR_WrtTime = getCurrentTime();
            int parentStartingBlock = getStartingBlock(0);
            // create and initialize new directory
            struct Directory * newDir = malloc(sizeof(struct Directory));
            strcpy(newDir->Directory[0].DIR_Name, ".");
            newDir->Directory[0].DIR_Attr = 16;
            newDir->Directory[0].DIR_NTRes = 0;
		        newDir->Directory[0].DIR_CrtTimeTenth = 0;
		        newDir->Directory[0].DIR_CrtTime = 0;
		        newDir->Directory[0].DIR_CrtDate = 0;
		        newDir->Directory[0].DIR_LstAccDate = 0;
            newDir->Directory[0].DIR_FstClusHI = newDirStartingBlock >> 8;
            newDir->Directory[0].DIR_FstClusLO = newDirStartingBlock % 256;
            newDir->Directory[0].DIR_FileSize = VCBptr->BytesPerSector;
            newDir->Directory[0].DIR_WrtTime = getCurrentTime();
            newDir->Directory[0].DIR_WrtDate = getCurrentDate();
            strcpy(newDir->Directory[1].DIR_Name, "..");
            newDir->Directory[1].DIR_Attr = searchDir->Directory[0].DIR_Attr;
            newDir->Directory[1].DIR_FstClusHI = searchDir->Directory[0].DIR_FstClusHI;
            newDir->Directory[1].DIR_FstClusLO = searchDir->Directory[0].DIR_FstClusLO;
            newDir->Directory[1].DIR_FileSize = searchDir->Directory[0].DIR_FileSize;
            newDir->Directory[1].DIR_WrtTime = searchDir->Directory[0].DIR_WrtTime;
            newDir->Directory[1].DIR_WrtDate = searchDir->Directory[0].DIR_WrtDate;
            LBAwrite(searchDir, 1, parentStartingBlock);
            LBAwrite(newDir, 1, newDirStartingBlock);
            resetSearch();
            printf("%s has been created.\n", pathname);
            return 0;
            }
          }
        }
      }
    }
  printf("Unable to make new directory\n");
  return 1;
  }

int fs_rmdir(const char *pathname)
  {
  isValidPath(pathname, 0);
  if(strcmp(pathname, "/") == 0)
    {
    printf("Unable to delete Root Directory.\n");
    return 0;
    }
  for(int i = 2; i < 16; i++)
    {
    if(searchDir->Directory[i].DIR_Name[0] != 0x00)
      {
      printf("Unable to delete directory %s - Directory must be empty.\n", pathname);
      return 0;
      }
    }
  int dirStartingBlock = getStartingBlock(0);
  int parentStartingBlock = getStartingBlock(1);
  // update parent directory
  struct Directory * parentDir;
  if(parentStartingBlock == VCBptr->RootCluster)
    {
    parentDir = ROOTptr;
    }
  else
    {
    parentDir = malloc(sizeof(struct Directory));
    LBAread(parentDir, 1, parentStartingBlock);
    }
  for(int i = 2; i < 16; i++)
    {
    if(strcmp(parentDir->Directory[i].DIR_Name, lastFileName) == 0)
      {
      // find DE corresponding to directory to be deleted, set as free
      parentDir->Directory[i].DIR_Name[0] = 0x00;
      break;
      }
    }
  parentDir->Directory[0].DIR_WrtTime = getCurrentTime();
  parentDir->Directory[0].DIR_WrtDate = getCurrentDate();
  LBAwrite(parentDir, 1, parentStartingBlock);
  releaseBlocks(dirStartingBlock, 512);
  printf("%s deleted.\n", pathname);
  if(searchDir == parentDir)
    {
    resetSearch();
    parentDir = NULL;
    }
  else
    {
    if(parentDir != ROOTptr)
      {
      free(parentDir);
      }
    parentDir = NULL;
    }
  return 0;
  }

fdDir * fs_opendir(const char *name)
  {
  if(isValidPath(name, 0) == 0)
    {
    fdDir * dirptr = malloc(sizeof(fdDir));
    int dirStartingBlock = getStartingBlock(0);
    dirptr->directoryStartLocation = dirStartingBlock;
    dirptr->dirEntryPosition = 0;
    readingDirectory = 1;
    return dirptr;
    }
  }

struct fs_diriteminfo *fs_readdir(fdDir *dirp)
  {
  struct fs_diriteminfo * dirEntry = malloc(sizeof(struct fs_diriteminfo));
  while(dirp->dirEntryPosition < 16)
    { // find all non-empty DEs
    if(searchDir->Directory[dirp->dirEntryPosition].DIR_Name[0] != 0x00)
      {
      strcpy(dirEntry->d_name, searchDir->Directory[dirp->dirEntryPosition].DIR_Name);
      dirp->dirEntryPosition++;
      return dirEntry;
      }
    else
      {
      dirp->dirEntryPosition++;
      }
    }
  return NULL;
  }

int fs_closedir(fdDir *dirp)
  {
  // clean up 
  readingDirectory = 0;
  free(dirp);
  dirp = NULL;
  resetSearch();
  return 0;
  }

int fs_stat(const char *path, struct fs_stat *buf)
  {
  for(int i = 0; i < 16; i++)
    { // find given file
    if(strcmp(searchDir->Directory[i].DIR_Name, path) == 0)
      {
      buf->st_size = searchDir->Directory[i].DIR_FileSize;
      }
    }
  return 0;
  }
// update current working directory
char * fs_getcwd(char *buf, size_t size)
  {
  strcpy(buf, "/");
  strcat(buf, cwd);
  return buf;
  }

// change directory
int fs_setcwd(char *buf)
  {
  if(strcmp(buf, "/") == 0)
    {
    if(cwdptr != ROOTptr)
      {
      free(cwdptr);
      cwdptr = NULL;
      cwdptr = ROOTptr;
      strcpy(cwd, "");
      }
    }
  else if(strcmp(buf, "..") == 0)
    {
    if(cwdptr == ROOTptr)
      {
      return 0;
      }
    else
      {
      int parentStartingBlock = cwdptr->Directory[1].DIR_FstClusHI << 8 |
                                cwdptr->Directory[1].DIR_FstClusLO;
      free(cwdptr);
      cwdptr = NULL;
      cwdptr = malloc(sizeof(struct Directory));
      LBAread(cwdptr, 1, parentStartingBlock);
      while(cwd[strlen(cwd) - 1] != '/' && strlen(cwd) > 1)
        {
        cwd[strlen(cwd) - 1] = 0;
        }
      cwd[strlen(cwd) - 1] = 0;
      }
    }
  else if(strcmp(buf, ".") == 0)
    {
    return 0;
    }
  else
    {
    if(isValidPath(buf, 0) != 0)
      {
      printf("Invalid path\n");
      return -1;
      }
    if(buf[strlen(buf)-1] == '/')
      {
      buf[strlen(buf)-1] = 0;
      }
    if(buf[0] == '/')
      {
      buf = buf + 1;
      strcpy(cwd, buf);
      }
    else
      {
      if(strcmp(cwd, "") != 0)
        {
        strcat(cwd, "/");
        }
      strcat(cwd, buf);
      }
    if(cwdptr != ROOTptr && cwdptr != searchDir)
      {
      free(cwdptr);
      }
    cwdptr = NULL;
    cwdptr = searchDir;
    }
  return 0;
  }

int fs_isFile(char * path)
  {
  if(isValidPath(path, 0) != 0)
    {
    resetSearch();
    return 0;
    }
  for(int i = 2; i < 16; i++)
    { // find file and check file/directory bit
    if(strcmp(searchDir->Directory[i].DIR_Name, lastFileName) == 0)
      {
      if(((searchDir->Directory[i].DIR_Attr % 32) >> 4) != 1)
        {
        resetSearch();
        return 1;
        }
      else
        {
        resetSearch();
        return 0;
        }
      }
    }
  }

int fs_isDir(char * path)
  {
  if(readingDirectory == 1)
    {
    for(int i = 0; i < 16; i++)
      { // find file and check file/directory bit
      if (strcmp(searchDir->Directory[i].DIR_Name, path) == 0)
        {
          return (((searchDir->Directory[i].DIR_Attr % 32) >> 4) == 1);
        }
      }
    }
  if(isValidPath(path, 1) != 0)
    {
    resetSearch();
    return 0;
    }
  struct Directory * parentDirectory = malloc(sizeof(struct Directory));
  parentDirectory = searchDir;
  if(isValidPath(path, 0) != 0)
    {
    resetSearch();
    return 0;
    }
  for(int i = 2; i < 16; i++)
    {
    if(strcmp(parentDirectory->Directory[i].DIR_Name, lastFileName) == 0)
      {
      if(((parentDirectory->Directory[i].DIR_Attr % 32) >> 4) == 1)
        {
        resetSearch();
        return 1;
        }
      else
        {
        resetSearch();
        return 0;
        }
      }
    }
  }

int fs_delete(char* filename)
  {
  int fileStartingBlock;
  int fileSize;
  for(int i = 2; i < 16; i++)
    { // find file, mark DE element as free, update parent directory
    if(strcmp(searchDir->Directory[i].DIR_Name, lastFileName) == 0)
      {
      fileStartingBlock = getStartingBlock(i);
      fileSize = searchDir->Directory[i].DIR_FileSize;
      searchDir->Directory[i].DIR_Name[0] = 0x00;
      }
    }
  releaseBlocks(fileStartingBlock, fileSize);
  searchDir->Directory[0].DIR_WrtTime = getCurrentTime();
  searchDir->Directory[0].DIR_WrtTime = getCurrentTime();
  LBAwrite(searchDir, 1, getStartingBlock(0));
  resetSearch();
  return 0;
  }

/* returns 0 if path is valid, 1 otherwise
* if parameter full == 0, checks if entire path is valid,
* otherwise, does not check last file
*/
int isValidPath(const char * path, int full)
  {
  char* pathArray[30];
  char pathName[DIRMAX_LEN];
  char* token;
  int pathLength = 0;
  searchDir = malloc(sizeof(struct Directory));
  if(strncmp(path, "/", 1) == 0)
    {
    strcpy(pathName, path);
    searchDir = ROOTptr;
    }
  else
    {
    strcpy(pathName, path);
    if(strlen(cwd) > 1)
      {
      searchDir = cwdptr;
      }
    else
      {
      searchDir = ROOTptr;
      }
    }
  token = strtok(pathName, "/");
  while(token != NULL)
    {
    pathArray[pathLength] = token;
    pathLength++;
    token = strtok(NULL, "/");
    }
  if( full != 0 )
    {
    pathLength--;
    }
  for(int i = 0; i < pathLength; i++)
    {
    strcpy(lastFileName, pathArray[i]);
    for(int j = 2; j < 16; j++)
      {
      if(strcmp(searchDir->Directory[j].DIR_Name, pathArray[i]) == 0)
        {
        if(((searchDir->Directory[j].DIR_Attr % 32) >> 4) == 1)
          {
          int clusterBlock = getStartingBlock(j);
          resetSearch();
          searchDir = malloc(sizeof(struct Directory));
          LBAread(searchDir, 1, clusterBlock);
          break;
          }
        else if(i < pathLength - 1)
          {
          return 1;
          }
        else
          {
          break;
          }
        }
      if(j == 15)
        {
        return 1;
        }
      }
    }
  return 0;
  }

void resetSearch()
  {
  if(searchDir != ROOTptr && searchDir != cwdptr)
    {
    free(searchDir);
    }
  }

int getStartingBlock(int DE)
  {
  int startingBlock;
  startingBlock = searchDir->Directory[DE].DIR_FstClusHI << 8 |
                  searchDir->Directory[DE].DIR_FstClusLO;
  return startingBlock;
  }

int allocateBlocks(uint64_t fileSize)
	{
	int requestedBlocks = fileSize / 512;
	int nextFreeCluster;
	if(fileSize % 512 > 0)
		{
		requestedBlocks++;
		}
	if(FSIptr->FSI_Free_Count < requestedBlocks)
		{
			return 0;
		}
	else
		{
		FSIptr->FSI_Free_Count -= requestedBlocks;
		nextFreeCluster = FSIptr->FSI_Nxt_Free;
		for(int i = 0; i < requestedBlocks; i++)
			{
			if(i == requestedBlocks - 1)
				{ // used for last block of file
				FATptr1->fat[nextFreeCluster + i] = 0xFFFFFFFF;
				FATptr2->fat[nextFreeCluster + i] = 0xFFFFFFFF;
				}
			else
				{ // used for files occupying multiple blocks on disk
				FATptr1->fat[nextFreeCluster + i] = nextFreeCluster + i + 1;
				FATptr2->fat[nextFreeCluster + i] = nextFreeCluster + i + 1;
				}
			}
		FSIptr->FSI_Nxt_Free += requestedBlocks;
		LBAwrite(FATptr1, VCBptr->FATSz32, FATptr1->startingBlock);
		LBAwrite(FATptr2, VCBptr->FATSz32, FATptr2->startingBlock);
		LBAwrite(FSIptr, 1, 1);
		return nextFreeCluster;
		}
	}

void releaseBlocks(uint64_t startingBlock, uint64_t fileSize)
  {
  int blocksDeleted = fileSize / 512;
  if(fileSize % 512 > 0)
    {
    blocksDeleted++;
    }
  for(int i = 0; i < blocksDeleted; i++)
    {
    FATptr1->fat[i + startingBlock] = 0;
    FATptr2->fat[i + startingBlock] = 0;
    }
  FSIptr->FSI_Free_Count += blocksDeleted;
  LBAwrite(FATptr1, VCBptr->FATSz32, FATptr1->startingBlock);
	LBAwrite(FATptr2, VCBptr->FATSz32, FATptr2->startingBlock);
	LBAwrite(FSIptr, 1, 1);
  }