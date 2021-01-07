// synchconsole.cc
#include "synchconsole.h"

static void SynchConsoleReadAvail(int c)
{ SynchConsole *synchconsole = (SynchConsole *)c; synchconsole->ReadAvail(); }
static void SynchConsoleWriteDone(int c)
{ SynchConsole *synchconsole = (SynchConsole *)c; synchconsole->WriteDone(); }

SynchConsole::SynchConsole(char *readFile, char *writeFile) {
    console = new Console(readFile, writeFile, SynchConsoleReadAvail, SynchConsoleWriteDone, (int)this);
    lock = new Lock("synch console");
    semaphoreReadAvail = new Semaphore("synch console read avail", 0);
    semaphoreWriteDone = new Semaphore("synch console write avail", 1);
}

SynchConsole::~SynchConsole() {
    delete console;
    delete lock;
    delete semaphoreReadAvail;
    delete semaphoreWriteDone;
}

void SynchConsole::PutChar(char ch) {
    lock->Acquire();
    console->PutChar(ch);
    semaphoreWriteDone->P();
    lock->Release();
}

char SynchConsole::GetChar() {
    lock->Acquire();
    semaphoreReadAvail->P();
    char ch = console->GetChar();
    lock->Release();
    return ch;
}

void SynchConsole::WriteDone() {
    semaphoreWriteDone->V();
}

void SynchConsole::ReadAvail() {
    semaphoreReadAvail->V();
}