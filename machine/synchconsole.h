// synchconsole.h
// 参考SynchDisk类实现SynchConsole类
// 在基本Console类上添加同步机制

#ifndef SYNCHCONSOLE_H
#define SYNCHCONSOLE_H

#include "copyright.h"
#include "console.h"
#include "synch.h"


class SynchConsole{
public:
    SynchConsole(char *readFile, char *writeFile);
    ~SynchConsole();

    void PutChar(char ch);          // 将ch写到显示器上
    char GetChar();                 // 从键盘上获取到输入，如果有，则返回；否则，返回EOF
    void WriteDone();               // I/O模拟的内部过程
    void ReadAvail();

private:
    Console* console;
    Lock* lock;
    Semaphore* semaphoreReadAvail;  // 同步读信号量
    Semaphore* semaphoreWriteDone; // 同步写信号量
};

#endif