/**************************************************************
* Class:  CSC-415-01 Spring 2022
* Names: John Santiago, Muhammed Nafees, Janelle Lara, Madina Ahmadzai
* Student IDs: 909606963, 921941329, 920156598, 921835158
* GitHub Name: aktails
* Group Name: MJ's
* Project: Basic File System
*
* File: fsInit.h
*
* Description: Interface for main driver of file system assignment.
*
**************************************************************/
#include <stdint.h>

#pragma pack(1)
struct VCB
  {
	uint8_t jmpBoot[3];
	char OEMName[8];
	uint16_t BytesPerSector;
	uint8_t SectorPerCluster;
	uint16_t RsvdSectorCount;
	uint8_t NumOfFATs;
	uint16_t RootEntryCount;
	uint16_t TotalSectors16;
	uint8_t Media;
	uint16_t FATSz16;
	uint16_t SectorsPerTrack;
	uint16_t NumberOfHeads;
	uint32_t HiddenSectors;
	uint32_t TotalSectors32;
	uint32_t FATSz32;
	uint16_t ExtFlags;
	uint16_t FSVer;
	uint32_t RootCluster;
	uint16_t FSInfo;
	uint16_t BkBootSector;
	uint8_t Reserved0[12];
	uint8_t DriverNumber;
	uint8_t Reserved1;
	uint8_t BootSig;
	uint32_t VolumeID;
	char VolumeLabel[11] ;
	char FileSystemType[8];
	uint8_t Reserved2[420];
	uint16_t Signature;
  };

struct FSInfo
	{
  uint32_t FSI_LeadSig;
  uint32_t FSI_Reserved1[120];
  uint32_t FSI_StrucSig;
  uint32_t FSI_Free_Count;
  uint32_t FSI_Nxt_Free;
  uint32_t FSI_Reserved2[3];
  uint32_t FSI_TrailSig;
	};

struct fsFat 
	{
  uint32_t fat[10000];
  uint32_t startingBlock;
	};

struct DirectoryEntry
  {
	char DIR_Name[11];
	uint8_t DIR_Attr;
	uint8_t DIR_NTRes;
	uint8_t DIR_CrtTimeTenth;
	uint16_t DIR_CrtTime;
	uint16_t DIR_CrtDate;
	uint16_t DIR_LstAccDate;
	uint16_t DIR_FstClusHI;
	uint16_t DIR_WrtTime;
	uint16_t DIR_WrtDate;
	uint16_t DIR_FstClusLO;
	uint32_t DIR_FileSize;
  };

struct Directory
  {
	struct DirectoryEntry Directory[16];
  };

extern struct VCB * VCBptr;
extern struct FSInfo * FSIptr;
extern struct fsFat *FATptr1, *FATptr2;
extern struct Directory * ROOTptr;
extern struct Directory * cwdptr;
extern struct Directory * searchDir;

int initVCB(uint64_t numberOfBlocks, uint64_t blockSize);
int initFAT(uint64_t numberOfBlocks, uint64_t blockSize);
int initRootDirectory();
uint16_t getCurrentTime();
uint16_t getCurrentDate();