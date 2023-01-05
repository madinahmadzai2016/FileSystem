/**************************************************************
* Class:  CSC-415-01 Spring 2022
* Names: John Santiago, Muhammed Nafees, Janelle Lara, Madina Ahmadzai
* Student IDs: 909606963, 921941329, 920156598, 921835158
* GitHub Name: aktails
* Group Name: MJ's
* Project: Basic File System
*
* File: fsInit.c
*
* Description: Main driver for file system assignment.
*
**************************************************************/


#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "fsLow.h"
#include "mfs.h"
#include "fsInit.h"

struct VCB * VCBptr;
struct FSInfo * FSIptr;
struct fsFat * FATptr1, * FATptr2;
struct Directory * ROOTptr;
struct Directory * cwdptr;
struct Directory * searchDir;


int initVCB(uint64_t numberOfBlocks, uint64_t blockSize)
	{
	VCBptr->jmpBoot[0] = 235;
	VCBptr->jmpBoot[1] = 0;
	VCBptr->jmpBoot[2] = 144;
	strcpy(VCBptr->OEMName, "MSDOS5.0");
	VCBptr->BytesPerSector = blockSize;
	VCBptr->SectorPerCluster = 1;
	VCBptr->RsvdSectorCount = 2;
	VCBptr->NumOfFATs = 2;
	VCBptr->RootEntryCount = 0;
	VCBptr->TotalSectors16 = 0;
	VCBptr->Media = 248;
	VCBptr->FATSz16 = 0;
	VCBptr->SectorsPerTrack = 0;
	VCBptr->NumberOfHeads = 0;
	VCBptr->HiddenSectors = 0;
	VCBptr->TotalSectors32 = numberOfBlocks;
	VCBptr->FATSz32 = 79;
	VCBptr->ExtFlags = 0;
	VCBptr->FSVer = 0;
	VCBptr->RootCluster = 2;
	VCBptr->FSInfo = 1;
	VCBptr->BkBootSector = 0;
	for (int i = 0 ; i < 12; i++)
		{
		VCBptr->Reserved0[i] = 0;
		}
	VCBptr->DriverNumber = 128;
	VCBptr->Reserved1 = 0;
	VCBptr->BootSig = 41;
	VCBptr->VolumeID = 1337;
	strcpy(VCBptr->VolumeLabel, "MJ'sFS");
	strcpy(VCBptr->FileSystemType, "FAT32");
	for(int i = 0 ; i < 420 ; i++)
		{
		VCBptr->Reserved2[420] = 0;
		}
	VCBptr->Signature = 43605;
	LBAwrite(VCBptr, 1, 0);
	return 0;
	}

int initFAT(uint64_t numberOfBlocks, uint64_t blockSize)
	{
  // allocate and initialize FSInfo struct
  FSIptr = malloc(blockSize); 

  FSIptr->FSI_LeadSig = 0x41615252;
  for (int i = 0; i < 120; i++)
  	{
    FSIptr->FSI_Reserved1[i] = 0;
  	}
  FSIptr->FSI_StrucSig = 0x61417272;
  FSIptr->FSI_Free_Count = numberOfBlocks - VCBptr->RsvdSectorCount; // -2 bc vcb and FSInfo take up 2 blocks
  FSIptr->FSI_Nxt_Free = VCBptr->RootCluster;
  for (int i = 0; i < 3; i++)
  	{
    FSIptr->FSI_Reserved2[i] = 0;
  	}
  FSIptr->FSI_TrailSig = 0xAA550000;

	LBAwrite(FSIptr, 1, 1);

  // FAT32 filesystem has 2 copies of the FAT
  FATptr1 = malloc(sizeof(struct fsFat));
	// calculate number of blocks the fat will use
  int FATSize = sizeof(struct fsFat) / blockSize;
	if (sizeof(struct fsFat) % blockSize > 0)
  	{
    FATSize++;
  	}
  FATptr1->fat[0] = 1; // reserved for VCB
  FATptr1->fat[1] = 1; // reserved for FSInfo block
  for (int i = 2; i < 10000; i++)
  	{
    FATptr1->fat[i] = 0;
  	}
	FATptr1->startingBlock = FSIptr->FSI_Nxt_Free;

  FATptr2 = malloc(sizeof(struct fsFat));
  FATptr2->fat[0] = 1; // reserved for VCB
  FATptr2->fat[1] = 1; // reserved for FSInfo block
  for (int i = 2; i < 10000; i++)
  	{
    FATptr2->fat[i] = 0;
  	}
	FATptr2->startingBlock = FSIptr->FSI_Nxt_Free + FATSize;

  for(int i = 0; i < FATSize; i++)
  	{
    if(i == FATSize - 1)
    	{
      FATptr1->fat[i + FATptr1->startingBlock] = 0xFFFFFFFF;
			FATptr2->fat[i + FATptr1->startingBlock] = 0xFFFFFFFF;
    	}
    else
    	{
      FATptr1->fat[i + FATptr1->startingBlock] = i + FATptr1->startingBlock + 1;
			FATptr2->fat[i + FATptr1->startingBlock] = i + FATptr1->startingBlock + 1;
    	}
  	}
  for(int i = 0; i < FATSize; i++)
  	{
    if(i == FATSize - 1)
    	{
      FATptr1->fat[i + FATptr2->startingBlock] = 0xFFFFFFFF;
			FATptr2->fat[i + FATptr2->startingBlock] = 0xFFFFFFFF;
    	}
    else
    	{
      FATptr1->fat[i + FATptr2->startingBlock] = i + FATptr2->startingBlock + 1;
			FATptr2->fat[i + FATptr2->startingBlock] = i + FATptr2->startingBlock + 1;
    	}
  	}
  FSIptr->FSI_Nxt_Free += (2 * FATSize);
  FSIptr->FSI_Free_Count -= (2 * FATSize);

	LBAwrite(FATptr1, FATSize, FATptr1->startingBlock);
	LBAwrite(FATptr2, FATSize, FATptr2->startingBlock);
	LBAwrite(FSIptr, 1, 1);

	return 0;
}

int initRootDirectory() {
	int rootStartingBlock = FSIptr->FSI_Nxt_Free;
	ROOTptr = malloc(sizeof(struct Directory));
	cwdptr = malloc(sizeof(struct Directory));
	searchDir = malloc(sizeof(struct Directory));
	cwdptr = ROOTptr;
	searchDir = ROOTptr;
	for(int i = 0; i < 2 ; i++) {
		if(i == 0) {
			strcpy(ROOTptr->Directory[i].DIR_Name, ".");
		}
		else {
			strcpy(ROOTptr->Directory[i].DIR_Name, "..");
		}
		ROOTptr->Directory[i].DIR_Attr = 16;
		ROOTptr->Directory[i].DIR_NTRes = 0;
		ROOTptr->Directory[i].DIR_CrtTimeTenth = 0;
		ROOTptr->Directory[i].DIR_CrtTime = 0;
		ROOTptr->Directory[i].DIR_CrtDate = 0;
		ROOTptr->Directory[i].DIR_LstAccDate = 0;
		ROOTptr->Directory[i].DIR_FstClusHI = rootStartingBlock >> 8;
		ROOTptr->Directory[i].DIR_FstClusLO = rootStartingBlock % 256;
		ROOTptr->Directory[i].DIR_FileSize = VCBptr->BytesPerSector;
		ROOTptr->Directory[i].DIR_WrtTime = getCurrentTime(time);
		ROOTptr->Directory[i].DIR_WrtDate = getCurrentDate(time);
		}
	VCBptr->RootCluster = rootStartingBlock;
	LBAwrite(VCBptr, VCBptr->BytesPerSector, 0);
	LBAwrite(ROOTptr, 1, rootStartingBlock);
	FATptr1->fat[rootStartingBlock] = 0xFFFFFFFF;
	FATptr2->fat[rootStartingBlock] = 0xFFFFFFFF;
	FSIptr->FSI_Nxt_Free += 1;
  FSIptr->FSI_Free_Count -= 1;
	LBAwrite(FATptr1, VCBptr->FATSz32, FATptr1->startingBlock);
	LBAwrite(FATptr2, VCBptr->FATSz32, FATptr2->startingBlock);

	LBAwrite(FSIptr, 1, 1);


	return 0;
}

uint16_t getCurrentTime()
	{
	time_t currTime = time(NULL);
	struct tm * time = localtime(&currTime);
	return (time->tm_sec / 2) | (time->tm_min << 5) | (time->tm_hour << 11);
	}

uint16_t getCurrentDate()
	{
	time_t currTime = time(NULL);
	struct tm * time = localtime(&currTime);
	return (time->tm_mday) | ((time->tm_mon + 1) << 5) | ((time->tm_year - 80) << 9);
	}

int initFileSystem (uint64_t numberOfBlocks, uint64_t blockSize)
	{
	printf ("Initializing File System with %ld blocks with a block size of %ld\n", numberOfBlocks, blockSize);
	/* TODO: Add any code you need to initialize your file system. */
	VCBptr = malloc(blockSize);
	LBAread(VCBptr, 1, 0);
	if (VCBptr->Signature != 43605)
		{
		initVCB(numberOfBlocks, blockSize);
		initFAT(numberOfBlocks, blockSize);
		initRootDirectory(blockSize);
		}
	else
		{
		FSIptr = malloc(sizeof(struct FSInfo));
		FATptr1 = malloc(blockSize * VCBptr->FATSz32);
		FATptr2 = malloc(blockSize * VCBptr->FATSz32);
		ROOTptr = malloc(sizeof(struct Directory));
		cwdptr = malloc(sizeof(struct Directory));
		LBAread(FSIptr, 1, 1);
		LBAread(FATptr1, VCBptr->FATSz32, 2);
		LBAread(FATptr2, VCBptr->FATSz32, 81);
		LBAread(ROOTptr, 1, VCBptr->RootCluster);
		cwdptr = ROOTptr;
		}

	return 0;
	}
	
void exitFileSystem ()
	{
	free(VCBptr);
	VCBptr = NULL;
	free(FSIptr);
	FSIptr = NULL;
	free(FATptr1);
	FATptr1 = NULL;
	free(FATptr2);
	FATptr2 = NULL;
	if(cwdptr != ROOTptr)
		{
		free(cwdptr);
		cwdptr = NULL;
		}
	free(ROOTptr);
	ROOTptr = NULL;
	printf ("System exiting\n");
	}