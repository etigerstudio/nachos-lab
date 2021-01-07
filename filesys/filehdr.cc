// filehdr.cc 
//	Routines for managing the disk file header (in UNIX, this
//	would be called the i-node).
//
//	The file header is used to locate where on disk the 
//	file's data is stored.  We implement this as a fixed size
//	table of pointers -- each entry in the table points to the 
//	disk sector containing that portion of the file data
//	(in other words, there are no indirect or doubly indirect 
//	blocks). The table size is chosen so that the file header
//	will be just big enough to fit in one disk sector, 
//
//      Unlike in a real system, we do not keep track of file permissions, 
//	ownership, last modification date, etc., in the file header. 
//
//	A file header can be initialized in two ways:
//	   for a new file, by modifying the in-memory data structure
//	     to point to the newly allocated data blocks
//	   for a file already on disk, by reading the file header from disk
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#include "system.h"
#include "filehdr.h"

#define LevelMapNum (SectorSize / sizeof(int))

//----------------------------------------------------------------------
// FileHeader::Allocate
// 	Initialize a fresh file header for a newly created file.
//	Allocate data blocks for the file out of the map of free disk blocks.
//	Return FALSE if there are not enough free blocks to accomodate
//	the new file.
//
//	"freeMap" is the bit map of free disk sectors
//	"fileSize" is the bit map of free disk sectors
//----------------------------------------------------------------------

bool
FileHeader::Allocate(BitMap *freeMap, int fileSize)
{ 
    numBytes = fileSize;
    numSectors  = divRoundUp(fileSize, SectorSize);
    if (freeMap->NumClear() < numSectors)
	    return FALSE;		// not enough space

    // 直接索引就够用
    if (numSectors < NumDirect) {
        //DEBUG('f', COLORED(OKGREEN, "Allocating using direct indexing only\n"));
        for (int i = 0; i < numSectors; i++)
            dataSectors[i] = freeMap->Find();
    }else {
        //文件长度小于7*128+32*128时，使用直接索引+二级索引
        if (numSectors < (NumDirect + LevelMapNum)) {
            DEBUG('f', "Allocating using single indirect indexing\n");
            // 直接索引
            for (int i = 0; i < NumDirect; i++)
                dataSectors[i] = freeMap->Find();
            // 二级索引
            dataSectors[IndirectSectorIdx] = freeMap->Find();
            int indirectIndex[LevelMapNum];
            for (int i = 0; i < numSectors - NumDirect; i++) {
                indirectIndex[i] = freeMap->Find();
            }
            synchDisk->WriteSector(dataSectors[IndirectSectorIdx], (char*)indirectIndex);
            //文件长度小于7*128+32*128+32*32*128时，使用直接索引+二级索引+三级索引
        } else if (numSectors < (NumDirect + LevelMapNum + LevelMapNum*LevelMapNum)) {
            DEBUG('f',"Allocating using double indirect indexing\n");
            // 直接索引
            for (int i = 0; i < NumDirect; i++)
                dataSectors[i] = freeMap->Find();
            dataSectors[IndirectSectorIdx] = freeMap->Find();
            // 二级索引
            int indirectIndex[LevelMapNum];
            for (int i = 0; i < LevelMapNum; i++) {
                indirectIndex[i] = freeMap->Find();
            }
            synchDisk->WriteSector(dataSectors[IndirectSectorIdx], (char*)indirectIndex);
            // 三级索引
            dataSectors[DoubleIndirectSectorIdx] = freeMap->Find();
            const int sectorsLeft = numSectors - NumDirect - LevelMapNum;
            const int secondIndirectNum = divRoundUp(sectorsLeft, LevelMapNum);
            int doubleIndirectIndex[LevelMapNum];
            
            for (int j = 0; j < secondIndirectNum; j++) {
                doubleIndirectIndex[j] = freeMap->Find();
                int singleIndirectIndex[LevelMapNum];
                for (int i = 0; (i < LevelMapNum) && (i + j * LevelMapNum < sectorsLeft); i++) {
                    singleIndirectIndex[i] = freeMap->Find();
                }
                synchDisk->WriteSector(doubleIndirectIndex[j], (char*)singleIndirectIndex);
            }
            synchDisk->WriteSector(dataSectors[DoubleIndirectSectorIdx], (char*)doubleIndirectIndex);
        } else {//超出文件最大长度，无法存储
            ASSERT(FALSE);
        }
    }
    // for (int i = 0; i < numSectors; i++)
	//     dataSectors[i] = freeMap->Find();
    return TRUE;
}

//----------------------------------------------------------------------
// FileHeader::Deallocate
// 	De-allocate all the space allocated for data blocks for this file.
//
//	"freeMap" is the bit map of free disk sectors
//----------------------------------------------------------------------

void 
FileHeader::Deallocate(BitMap *freeMap)
{
    // 分别对应直接索引、二级索引、三级索引
    int i, ii, iii;
    DEBUG('f',"Deallocating direct indexing table\n");
    for (i = 0; (i < numSectors) && (i < NumDirect); i++) {
        ASSERT(freeMap->Test((int)dataSectors[i]));
        freeMap->Clear((int)dataSectors[i]);
    }
    if (numSectors > NumDirect) {
        DEBUG('f', "Deallocating single indirect indexing table\n");
        int singleIndirectIndex[LevelMapNum]; 
        synchDisk->ReadSector(dataSectors[IndirectSectorIdx], (char*)singleIndirectIndex);
        //回收二级索引存储下的文件
        for (i = NumDirect, ii = 0; (i < numSectors) && (ii < LevelMapNum); i++, ii++) {
            ASSERT(freeMap->Test((int)singleIndirectIndex[ii]));
            freeMap->Clear((int)singleIndirectIndex[ii]);
        }
        ASSERT(freeMap->Test((int)dataSectors[IndirectSectorIdx]));
        freeMap->Clear((int)dataSectors[IndirectSectorIdx]);
        //回收三级索引存储下的文件
        if (numSectors > NumDirect + LevelMapNum) {
            DEBUG('f',"Deallocating double indirect indexing table\n");
            int doubleIndirectIndex[LevelMapNum];
            synchDisk->ReadSector(dataSectors[DoubleIndirectSectorIdx], (char*)doubleIndirectIndex);
            for (i = NumDirect + LevelMapNum, ii = 0; (i < numSectors) && (ii < LevelMapNum); ii++) {
                synchDisk->ReadSector(doubleIndirectIndex[ii], (char*)singleIndirectIndex);
                for (iii = 0; (i < numSectors) && (iii < LevelMapNum); i++, iii++) {
                    ASSERT(freeMap->Test((int)singleIndirectIndex[iii])); 
                    freeMap->Clear((int)singleIndirectIndex[iii]);
                }
                ASSERT(freeMap->Test((int)doubleIndirectIndex[ii]));
                freeMap->Clear((int)doubleIndirectIndex[ii]);
            }
            ASSERT(freeMap->Test((int)dataSectors[DoubleIndirectSectorIdx]));
            freeMap->Clear((int)dataSectors[DoubleIndirectSectorIdx]);
        }
    }
}

//----------------------------------------------------------------------
// FileHeader::FetchFrom
// 	Fetch contents of file header from disk. 
//
//	"sector" is the disk sector containing the file header
//----------------------------------------------------------------------

void
FileHeader::FetchFrom(int sector)
{
    synchDisk->ReadSector(sector, (char *)this);
}

//----------------------------------------------------------------------
// FileHeader::WriteBack
// 	Write the modified contents of the file header back to disk. 
//
//	"sector" is the disk sector to contain the file header
//----------------------------------------------------------------------

void
FileHeader::WriteBack(int sector)
{
    synchDisk->WriteSector(sector, (char *)this); 
}

//----------------------------------------------------------------------
// FileHeader::ByteToSector
// 	Return which disk sector is storing a particular byte within the file.
//      This is essentially a translation from a virtual address (the
//	offset in the file) to a physical address (the sector where the
//	data at the offset is stored).
//
//	"offset" is the location within the file of the byte in question
//----------------------------------------------------------------------

// 当我们使用synchDisk->ReadSector和synchDisk->WriteSector的时候则会使用到该函数
// 如果计算错误则会报错“Assertion failed: line 121, file "../machine/disk.cc"”
int
FileHeader::ByteToSector(int offset)
{
    const int directMapSize = NumDirect * SectorSize;
    const int singleIndirectMapSize = directMapSize + LevelMapNum * SectorSize;
    const int doubleIndirectMapSize = singleIndirectMapSize +  LevelMapNum * LevelMapNum * SectorSize;
    if (offset < directMapSize) {
        return (dataSectors[offset / SectorSize]);
    } else if (offset < singleIndirectMapSize) {
        const int sectorNum = (offset - directMapSize) / SectorSize;
        int singleIndirectIndex[LevelMapNum];
        synchDisk->ReadSector(dataSectors[IndirectSectorIdx], (char*)singleIndirectIndex);
        return singleIndirectIndex[sectorNum];
    } else {
        const int indexSectorNum = (offset - singleIndirectMapSize) / SectorSize / LevelMapNum;
        const int sectorNum = (offset - singleIndirectMapSize) / SectorSize % LevelMapNum;
        int doubleIndirectIndex[LevelMapNum]; 
        synchDisk->ReadSector(dataSectors[DoubleIndirectSectorIdx], (char*)doubleIndirectIndex);
        int singleIndirectIndex[LevelMapNum]; 
        synchDisk->ReadSector(doubleIndirectIndex[indexSectorNum], (char*)singleIndirectIndex);
        return singleIndirectIndex[sectorNum];
    }
}

//----------------------------------------------------------------------
// FileHeader::FileLength
// 	Return the number of bytes in the file.
//----------------------------------------------------------------------

int
FileHeader::FileLength()
{
    return numBytes;
}

//----------------------------------------------------------------------
// FileHeader::Print
// 	Print the contents of the file header, and the contents of all
//	the data blocks pointed to by the file header.
//----------------------------------------------------------------------

void
FileHeader::Print()
{
    int i, j, k;
    char *data = new char[SectorSize];
    printf("--------------------- %s ----------------------\n", "FileHeader contents");
    printf("\tFileType type: %s\n", fileType);
    printf("\tCreated: %s ", createdTime);
    printf("\tModified: %s ", modifiedTime);
    printf("\tLast Visited Time: %s ", lastVisitedTime);
    // printf("\tPath: %s\n", filePath);

    printf("File size: %d.  File blocks:\n", numBytes);

    // 与上面的对应
    int ii, iii;
    int singleIndirectIndex[LevelMapNum];
    int doubleIndirectIndex[LevelMapNum];
    printf("\tDirect indexing:\n\t");

    for (i = 0; (i < numSectors) && (i < NumDirect); i++)
	    printf("%d ", dataSectors[i]);
    
    if (numSectors > NumDirect) {
        printf("\n  Indirect indexing: (mapping table sector: %d)\n    ", dataSectors[IndirectSectorIdx]);
        synchDisk->ReadSector(dataSectors[IndirectSectorIdx], (char*)singleIndirectIndex);
        for (i = NumDirect, ii = 0; (i < numSectors) && (ii < LevelMapNum); i++, ii++)
            printf("%d ", singleIndirectIndex[ii]);
        if (numSectors > NumDirect + LevelMapNum) {
            printf("\n  Double indirect indexing: (mapping table sector: %d)", dataSectors[DoubleIndirectSectorIdx]);
            synchDisk->ReadSector(dataSectors[DoubleIndirectSectorIdx], (char*)doubleIndirectIndex);
            for (i = NumDirect + LevelMapNum, ii = 0; (i < numSectors) && (ii < LevelMapNum); ii++) {
                printf("\n    single indirect indexing: (mapping table sector: %d)\n      ", doubleIndirectIndex[ii]);
                synchDisk->ReadSector(doubleIndirectIndex[ii], (char*)singleIndirectIndex);
                for (iii = 0;  (i < numSectors) && (iii < LevelMapNum); i++, iii++)
                    printf("%d ", singleIndirectIndex[iii]);
            }
        }
    }

    printf("\nFile contents:\n");
    for (i = k = 0; (i < numSectors) && (i < NumDirect); i++)
    {
        synchDisk->ReadSector(dataSectors[i], data);
        for (j = 0; (j < SectorSize) && (k < numBytes); j++, k++)
            printChar(data[j]);
        printf("\n");
    }
    if (numSectors > NumDirect) {
        synchDisk->ReadSector(dataSectors[IndirectSectorIdx], (char*)singleIndirectIndex);
        for (i = NumDirect, ii = 0; (i < numSectors) && (ii < LevelMapNum); i++, ii++) {
            synchDisk->ReadSector(singleIndirectIndex[ii], data);
            for (j = 0; (j < SectorSize) && (k < numBytes); j++, k++)
                printChar(data[j]);
            printf("\n");
        }
        if (numSectors > NumDirect + LevelMapNum) {
            synchDisk->ReadSector(dataSectors[DoubleIndirectSectorIdx], (char*)doubleIndirectIndex);
            for (i = NumDirect + LevelMapNum, ii = 0; (i < numSectors) && (ii < LevelMapNum); ii++) {
                synchDisk->ReadSector(doubleIndirectIndex[ii], (char*)singleIndirectIndex);
                for (iii = 0; (i < numSectors) && (iii < LevelMapNum); i++, iii++) {
                    synchDisk->ReadSector(singleIndirectIndex[iii], data);
                    for (j = 0; (j < SectorSize) && (k < numBytes); j++, k++)
                        printChar(data[j]);
                    printf("\n");
                }
            }
        }
    }
    printf("----------------------------------------------\n");
    delete[] data;
}

char*
printChar(char oriChar)
{
    if ('\040' <= oriChar && oriChar <= '\176') 
        printf("%c", oriChar); 
    else
        printf("\\%x", (unsigned char)oriChar); 
}

//----------------------------------------------------------------------
// getFileExtension
//    Extract the file name to get the extension. If the file name don't
//    have extension then return empty string. 
//
//      e.g. test.haha.pdf => "pdf"
//      e.g. test.txt      => txt
//      e.g. test.         => ""
//      e.g. test          => ""
//----------------------------------------------------------------------

char*
getFileExtension(char *filename)
{
    char *dot = strrchr(filename, '.');
    if(!dot || dot == filename) return "";
    return dot + 1;
}

//----------------------------------------------------------------------
// getCurrentTime
//    Return the sting of the time that we called it.
//
//    (use asctime to transfer to string)
//----------------------------------------------------------------------

char*
getCurrentTime(void)
{
    time_t rawtime;
    time(&rawtime);
    struct tm* currentTime = localtime(&rawtime);
    return asctime(currentTime); // This somehow will generate extra '\n'
}

//----------------------------------------------------------------------
// FileHeader::HeaderCreateInit
//  Set the file type, time informations and other attribute.
//  Invoke this when create a FileHeader first time.
//  (not every "new FileHeader")
//----------------------------------------------------------------------

void
FileHeader::HeaderCreateInit(char* ext)
{
    setFileType(ext);

    char* currentTimeString = getCurrentTime();
    setCreateTime(currentTimeString);
    setModifyTime(currentTimeString);
    setVisitTime(currentTimeString);
}

//----------------------------------------------------------------------
//  FilePath pathParser(char* path)
//  解析文件路径
//----------------------------------------------------------------------
FilePath pathParser(char* path) {
     //去除根
    if (path[0] == '/')
        path = &path[1];
    //拷贝路径
    char* ts1 = strdup(path);
    char* ts2 = strdup(path);
    FilePath filepath;
    //去除不是目录名的部分，只留下目录
    char* currentDir = dirname(ts1);
    //获取文件名部分
    filepath.base = strdup(basename(ts2));
    //统计目录深度
    int depth;
    for (depth = 0; path[depth]; path[depth] == '/' ? depth++ : *path++);
    filepath.dirDepth = depth;
    
    ASSERT(depth <= MAX_DIR_DEPTH);
    //前往当前目录
    while (strcmp(currentDir, ".")) { 
        filepath.dirArray[--depth] = strdup(basename(currentDir));
        currentDir = dirname(currentDir);
    }
    return filepath;
}


// Lab5: dynamic allocate file size

//----------------------------------------------------------------------
// FileHeader::ExpandFileSize
// 	Reallocate the file size for additionalBytes
//----------------------------------------------------------------------
#define INDIRECT_MAP
bool FileHeader::ExpandFileSize(BitMap* freeMap, int additionalBytes) {
    ASSERT(additionalBytes > 0);
    numBytes += additionalBytes;
    // 获取增长前扇区的数量
    int initSector = numSectors;
    // 计算增长后扇区的数量
    numSectors = divRoundUp(numBytes, SectorSize);
    // 相同则不需要额外的空间
    if (initSector == numSectors) {
        return TRUE; // no need more sector
    }
    int sectorsToExpand = numSectors - initSector;
    if (freeMap->NumClear() < sectorsToExpand) {
        // 没有空间
        return FALSE; // no more space to allocate
    }

    DEBUG('f', COLORED(OKGREEN, "Expanding file size for %d sectors (%d bytes)\n"), sectorsToExpand, additionalBytes);

    if (numSectors < NumDirect) { // just like FileHeader::Allocate
        // 有空间则继续分配
        for (int i = initSector; i < numSectors; i++)
            dataSectors[i] = freeMap->Find();
    } else {
#ifndef INDIRECT_MAP
        ASSERT_MSG(FALSE, "File size exceeded the maximum representation of the direct map");
#else

// TODO: Expand file size in indirect mapping mode

        if (numSectors < (NumDirect + LevelMapNum)) {
        } else if (numSectors < (NumDirect + LevelMapNum + LevelMapNum*LevelMapNum)) {
        } else {
            ASSERT_MSG(FALSE, "File size exceeded the maximum representation of the double indirect mapping");
        }
#endif
    }
    return TRUE;
}