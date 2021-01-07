// synchdisk.h 
// 	Data structures to export a synchronous interface to the raw 
//	disk device.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#ifndef SYNCHDISK_H
#define SYNCHDISK_H

#include "disk.h"
#include "synch.h"

// The following class defines a "synchronous" disk abstraction.
// As with other I/O devices, the raw physical disk is an asynchronous device --
// requests to read or write portions of the disk return immediately,
// and an interrupt occurs later to signal that the operation completed.
// (Also, the physical characteristics of the disk device assume that
// only one operation can be requested at a time).
//
// This class provides the abstraction that for any individual thread
// making a request, it waits around until the operation finishes before
// returning.
class SynchDisk {
  public:
    SynchDisk(char* name);    		// Initialize a synchronous disk,
					                        // by initializing the raw Disk.
    ~SynchDisk();			            // De-allocate the synch disk data
    
    void ReadSector(int sectorNumber, char* data);
                                  // Read/write a disk sector, returning
                                  // only once the data is actually read 
                                  // or written.  These call
    					                    // Disk::ReadRequest/WriteRequest and
					                        // then wait until the request is done.
    void WriteSector(int sectorNumber, char* data);
    
    void RequestDone();			      // Called by the disk device interrupt
                                  // handler, to signal that the
                                  // current disk operation is complete.
    // 读者读时申请锁
    void PlusReader(int sector);
    // 读者结束读时释放锁
    void MinusReader(int sector);
    //写者开始写时申请锁
    void BeginWrite(int sector);
    //写者结束写时释放锁
    void EndWrite(int sector);
    // 访问当前磁盘扇区的线程
    int numVisitors[NumSectors];

  private:
    Disk *disk;		  		          // Raw disk device
    Semaphore *semaphore; 		    // To synchronize requesting thread 
					                        // with the interrupt handler
    Lock *lock;		  		          // Only one read/write request
					                        // can be sent to the disk at a time
    // 文件访问信号量
    Semaphore *mutex[NumSectors];
    // 读者数量
    int numReaders[NumSectors];
    // 读者数量锁
    Lock* readerLock;
};

#endif // SYNCHDISK_H
