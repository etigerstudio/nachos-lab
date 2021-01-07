// exception.cc 
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.  
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "syscall.h"
// #include "filesys.h"
// #include "openfile.h"

//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2. 
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//	"which" is the kind of exception.  The list of possible exceptions 
//	are in machine.h.
//----------------------------------------------------------------------

void exec_func(int);

void
ExceptionHandler(ExceptionType which) {
    int type = machine->ReadRegister(2);

    if (which == SyscallException) {
        switch (type) {
            case SC_Halt: {
                DEBUG('a', "Shutdown, initiated by user program.\n");
                interrupt->Halt();
                break;
            }
            case SC_Create: {
                int address = machine->ReadRegister(4);
                char name[10];
                int pos = 0, data;
                while (1) {
                    machine->ReadMem(address + pos, 1, &data);
                    if (data == 0) {
                        name[pos] = '\0';
                        break;
                    }
                    name[pos++] = (char) data;
                }
                fileSystem->Create(name, 128);
                machine->AdvancePC();
                DEBUG(SYSCALL_DEBUG, "SYSCALL: Creating a file, name: %s\n", name);
                break;
            }
            case SC_Open: {
                int address = machine->ReadRegister(4);
                char name[10];
                int pos = 0, data;
                while (1) {
                    machine->ReadMem(address + pos, 1, &data);
                    if (data == 0) {
                        name[pos] = '\0';
                        break;
                    }
                    name[pos++] = (char) data;
                }
                OpenFile *openfile = fileSystem->Open(name);
                machine->WriteRegister(2, int(openfile));
                machine->AdvancePC();
                DEBUG(SYSCALL_DEBUG, "SYSCALL: Opened a file, name: %s\n", name);
                break;
            }
            case SC_Close: {
                int fd = machine->ReadRegister(4);
                OpenFile *openfile = (OpenFile *) fd;
                delete openfile;
                machine->AdvancePC();
                DEBUG(SYSCALL_DEBUG, "SYSCALL: Closed a file, id: %d\n", fd);
                break;
            }
            case SC_Read: {
                int buffer = machine->ReadRegister(4);
                int size = machine->ReadRegister(5);
                int fd = machine->ReadRegister(6);
                OpenFile *openfile = (OpenFile *) fd;

                if (fd == ConsoleInput) {
                    for (int i = 0; i < size; ++i)
                        machine->WriteMem(buffer + i, 1, int(getchar()));
                    machine->WriteRegister(2, size);
                    DEBUG(SYSCALL_DEBUG, "SYSCALL: Read from stdin, bytes read: %d\n", size);
                } else {
                    char content[size];
                    int result = openfile->Read(content, size);
                    for (int i = 0; i < result; ++i)
                        machine->WriteMem(buffer + i, 1, int(content[i]));
                    machine->WriteRegister(2, result);
                    DEBUG(SYSCALL_DEBUG, "SYSCALL: Read a file, bytes read: %d\n", result);
                }
                machine->AdvancePC();
                break;
            }
            case SC_Write: {
                int buffer = machine->ReadRegister(4);
                int size = machine->ReadRegister(5);
                int fd = machine->ReadRegister(6);
                char content[size];
                int data;
                printf("SYSCALL: Wrote buffer %d %d %d\n", buffer, size, fd);
                for (int i = 0; i < size; ++i) {
                    machine->ReadMem(buffer + i, 1, &data);
                    content[i] = char(data);
                }
                if (fd == ConsoleOutput) {
                    for (int i = 0; i < size; ++i)
                        putchar(content[i]);
                    DEBUG(SYSCALL_DEBUG, "SYSCALL: Wrote to stdout, bytes written: %d\n", size);
                } else {
                    OpenFile *openfile = (OpenFile *) fd;
                    openfile->Write(content, size);
                    DEBUG(SYSCALL_DEBUG, "SYSCALL: Wrote a file, bytes written: %d\n", size);
                }
                machine->AdvancePC();
                break;
            }
            case SC_Exec: {
                int address = machine->ReadRegister(4);
                Thread *newThread = new Thread("new thread");
                newThread->Fork(exec_func, address);
                machine->WriteRegister(2, newThread->getTID());
                machine->AdvancePC();
                DEBUG(SYSCALL_DEBUG, "SYSCALL: exec\n");
                break;
            }
            case SC_Yield: {
                DEBUG(SYSCALL_DEBUG, "SYSCALL: yield\n");
                machine->AdvancePC();
                currentThread->Yield();
                break;
            }
            case SC_Join: {
                DEBUG(SYSCALL_DEBUG, "SYSCALL: join\n");
                int tid = machine->ReadRegister(4);
                while (allThreads[tid])
                    currentThread->Yield();
                machine->AdvancePC();
                break;
            }
            case SC_Exit: {
                int status = machine->ReadRegister(4);
                printf("SYSCALL: exit, code: %d\n", status);
                machine->DeallocPageTable();
                machine->AdvancePC();
                currentThread->Finish();
                break;
            }
            case SC_Pwd: {
                #ifdef FILESYS_STUB
                system("pwd");
                #else
                currentThread->pwd();
                #endif
                machine->AdvancePC();
                break;
            }
            case SC_Ls: {
                #ifdef FILESYS_STUB
                system("ls");
                #endif
                machine->AdvancePC();
                break;
            }
            case SC_Cd: {
                #ifdef FILESYS_STUB
                int address = machine->ReadRegister(4);
                char name[20];
                int pos = 0;
                int data;
                while(1) {
                    machine->ReadMem(address + pos, 1, &data);
                    if (data == 0) {
                        name[pos] = '\0';
                        break;
                    }
                    name[pos++] = (char) data;
                }
                chdir(name);
                #endif
                machine->AdvancePC();
                break;
            }
            case SC_Remove: {
                #ifdef FILESYS_STUB
                int address = machine->ReadRegister(4);
                char name[20];
                int pos = 0;
                int data;
                while(1) {
                    machine->ReadMem(address + pos, 1, &data);
                    if (data == 0) {
                        name[pos] = '\0';
                        break;
                    }
                    name[pos++] = (char) data;
                }
                fileSystem->Remove(name);
                #endif
                machine->AdvancePC();
                break;
            }
            case SC_MkDir: {
                #ifdef FILESYS_STUB
                int address = machine->ReadRegister(4);
                char name[20];
                int pos = 0;
                int data;
                while(1) {
                    machine->ReadMem(address + pos, 1, &data);
                    if (data == 0) {
                        name[pos] = '\0';
                        break;
                    }
                    name[pos++] = (char) data;
                }
                mkdir(name, 0777);
                #endif
                machine->AdvancePC();
                break;
            }
            case SC_RmDir: {
                #ifdef FILESYS_STUB
                int address = machine->ReadRegister(4);
                char name[20];
                int pos = 0;
                int data;
                while(1) {
                    machine->ReadMem(address + pos, 1, &data);
                    if (data == 0) {
                        name[pos] = '\0';
                        break;
                    }
                    name[pos++] = (char) data;
                }
                rmdir(name);
                #endif
                machine->AdvancePC();
                break;
            }
            case SC_Help: {
                printf(COLORED(OKGREEN, "------------------------------[Nachos Shell Help]---------------------------------\n"));
                printf(COLORED(OKBLUE, "exec [-userprog] :\texecute user program\n"));
                // printf("\texecute user program\n");
                printf(COLORED(OKBLUE, "pwd : \t\t\tprint current path\n"));
                printf(COLORED(OKBLUE, "ls : \t\t\tlist the files and folders in current path\n"));
                printf(COLORED(OKBLUE, "touch [-filename] : \tcreate a new file\n"));
                printf(COLORED(OKBLUE, "mkdir [-dirname] : \tcreate a dir\n"));
                printf(COLORED(OKBLUE, "rm [-filename] : \tdelete a file\n"));
                printf(COLORED(OKBLUE, "rmdir [-dirname] : \tdelete a directory\n"));
                printf(COLORED(OKBLUE, "uptime : \t\tprint the system statistics up to now\n"));
                printf(COLORED(OKBLUE, "help/? : \t\tprint the help information\n"));
                printf(COLORED(OKBLUE, "exit/quit : \t\texit or quit the shell program\n"));
                printf(COLORED(OKBLUE, "halt : \t\t\thalt Nachos machine\n"));
                printf(COLORED(OKGREEN, "----------------------------------[Help End ]--------------------------------------\n"));
                machine->AdvancePC();
                break;
            }
            case SC_Uptime: {
                stats->Print();
                machine->AdvancePC();
                break;
            }
        }
    } else {
        printf("Unexpected user mode exception %d %d\n", which, type);
        ASSERT(FALSE);
    }
}

void Machine::AdvancePC() {
    WriteRegister(PrevPCReg, registers[PCReg]);
    WriteRegister(PCReg, registers[PCReg] + sizeof(int));
    WriteRegister(NextPCReg, registers[NextPCReg] + sizeof(int));
}

void exec_func(int address) {
    char name[10];
    int pos = 0, data;
    while (1) {
        machine->ReadMem(address + pos, 1, &data);
        if (data == 0) {
            name[pos] = '\0';
            break;
        }
        name[pos++] = (char) data;
    }
    OpenFile *executable = fileSystem->Open(name);
    AddrSpace *space;
    space = new AddrSpace(executable);
    currentThread->space = space;
    delete executable;
    space->InitRegisters();
    space->RestoreState();
    machine->Run();
}