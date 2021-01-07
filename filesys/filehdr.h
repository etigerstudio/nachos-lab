// filehdr.h 
//	Data structures for managing a disk file header.  
//
//	A file header describes where on disk to find the data in a file,
//	along with other information about the file (for instance, its
//	length, owner, etc.)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#ifndef FILEHDR_H
#define FILEHDR_H

#include "disk.h"
#include "bitmap.h"

#include <libgen.h>

#define MAX_DIR_DEPTH 5

//存储数据区域的扇区数量，也就是原来的直接索引数量
#define NumDataSectors ((SectorSize - (NumOfIntHeaderInfo*sizeof(int) + LengthOfAllString*sizeof(char))) / sizeof(int))
//直接索引数量
#define NumDirect (NumDataSectors - 2)
//二级索引的位置为一级索引的倒数第二个
#define IndirectSectorIdx (NumDataSectors - 2)
//三级索引的位置为一级索引的倒数第一个
#define DoubleIndirectSectorIdx (NumDataSectors - 1)
//重新定义最大文件长度
#define MaxFileSize (NumDirect * SectorSize) + \
                    ((SectorSize / sizeof(int)) * SectorSize) + \
                    ((SectorSize / sizeof(int)) * ((SectorSize / sizeof(int)) * SectorSize))


// Disk part
#define NumOfIntHeaderInfo 2
#define NumOfTimeHeaderInfo 3
#define LengthOfTimeHeaderStr 26 // 25 + 1 ('/0')
#define MaxExtLength 5           // 4  + 1 ('/0')
#define LengthOfAllString MaxExtLength + NumOfTimeHeaderInfo*LengthOfTimeHeaderStr

//直接索引个数，添加晚上上述信息之后，索引将由原来的30个变为9个
// #define NumDirect 	((SectorSize - (NumOfIntHeaderInfo*sizeof(int) + LengthOfAllString*sizeof(char))) / sizeof(int))

// The following class defines the Nachos "file header" (in UNIX terms,  
// the "i-node"), describing where on disk to find all of the data in the file.
// The file header is organized as a simple table of pointers to
// data blocks. 
//
// The file header data structure can be stored in memory or on disk.
// When it is on disk, it is stored in a single sector -- this means
// that we assume the size of this data structure to be the same
// as one disk sector.  Without indirect addressing, this
// limits the maximum file length to just under 4K bytes.
//
// There is no constructor; rather the file header can be initialized
// by allocating blocks for the file (if it is a new file), or by
// reading it from disk.

class FileHeader {
  public:
    bool Allocate(BitMap *bitMap, int fileSize);// Initialize a file header, 
						//  including allocating space 
						//  on disk for the file data
    void Deallocate(BitMap *bitMap);  		// De-allocate this file's 
						//  data blocks

    void FetchFrom(int sectorNumber); 	// Initialize file header from disk
    void WriteBack(int sectorNumber); 	// Write modifications to file header
					//  back to disk

    int ByteToSector(int offset);	// Convert a byte offset into the file
					// to the disk sector containing
					// the byte

    int FileLength();			// Return the length of the file 
					// in bytes

    void Print();			// Print the contents of the file.

    // Lab5: additional file attributes
    void HeaderCreateInit(char* ext); // Initialize all header message for creation
    // Disk part
    void setFileType(char* ext) { strcmp(ext, "") ? strcpy(fileType, ext) : strcpy(fileType, "None"); }
    void setCreateTime(char* t) { strcpy(createdTime, t); }
    void setModifyTime(char* t) { strcpy(modifiedTime, t); }
    void setVisitTime(char* t) { strcpy(lastVisitedTime, t); }
    // In-core part
    void setHeaderSector(int sector) { headerSector = sector; }
    int getHeaderSector() { return headerSector; }

    char* getFileType() { return strdup(fileType); }

    // Lab5: expand file size
    bool ExpandFileSize(BitMap *freeMap, int additionalBytes);

    int getNumBytes() { return numBytes; }
    void setNumBytes(int n) { numBytes = n; }

  private:
    // ======================== Disk Part ======================== //
    // == Header Information == //
    int numBytes;   // Number of bytes in the file
    int numSectors; // Number of data sectors in the file

    // Lab5: additional file attributes
    char fileType[MaxExtLength];
    char createdTime[LengthOfTimeHeaderStr];
    char modifiedTime[LengthOfTimeHeaderStr];
    char lastVisitedTime[LengthOfTimeHeaderStr];

    // == Data Sectors == //
    // int dataSectors[NumDirect]; // Disk sector numbers for each data
                                // block in the file
    int dataSectors[NumSectors];
    // ======================== In-core Part ======================== //
    // This will be assign value when the file is open!
    int headerSector; // Because when we OpenFile, we need to update the header information
                      // but the sector message is only exist when create the OpenFile object
                      // some how we need to know which sector to write back
};

char* printChar(char oriChar);

extern char* getFileExtension(char *filename);
extern char* getCurrentTime(void);

typedef struct {
    char* dirArray[MAX_DIR_DEPTH];
    // 定义目录深度，如果为0，则为根目录
    int dirDepth;
    char* base;
} FilePath;

//实现文件路径解析功能
extern FilePath pathParser(char* path);

#endif // FILEHDR_H
