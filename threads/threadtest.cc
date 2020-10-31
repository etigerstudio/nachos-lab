// threadtest.cc 
//	Simple test case for the threads assignment.
//
//	Create two threads, and have them context switch
//	back and forth between themselves by calling Thread::Yield, 
//	to illustratethe inner workings of the thread system.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"

// testnum is set in main.cc
int testnum = 1;

//----------------------------------------------------------------------
// SimpleThread
// 	Loop 5 times, yielding the CPU to another ready thread 
//	each iteration.
//
//	"which" is simply a number identifying the thread, for debugging
//	purposes.
//----------------------------------------------------------------------

void
SimpleThread(int which)
{
    int num;

    for (num = 0; num < 5; num++) {
        printf("*** thread %d looped %d times\n", which, num);
        currentThread->Yield();
    }
}

void
HighInLowThread(int which)
{
    int num;

    for (num = 0; num < 5; num++) {
        printf("*** thread %d looped %d times\n", which, num);
        if (num == 3) {
            Thread* t = new Thread("T1-4", 4);
            t->Fork(SimpleThread, 1);
        }
        currentThread->Yield();
    }
}

//----------------------------------------------------------------------
// ThreadTest1
// 	Set up a ping-pong between two threads, by forking a thread 
//	to call SimpleThread, and then calling SimpleThread ourselves.
//----------------------------------------------------------------------

void
ThreadTest1()
{
    DEBUG('t', "Entering ThreadTest1");

    Thread *t = new Thread("forked thread");

    t->Fork(SimpleThread, 1);
    SimpleThread(0);
}

//----------------------------------------------------------------------
// ThreadTest2
// 	Continously creating 128 new threads, expectedly resulting in
//  triggering assertion for exceeding max thread number.
//----------------------------------------------------------------------

void
ThreadTest2()
{
    DEBUG('t', "Entering ThreadTest2\n");

    for (int i = 0; i < MAX_THREAD_COUNT; i++) {
        char name[13];
        sprintf(name, "%dth thread", i + 2);
        Thread *t = new Thread(name);
    }

    printf("128 threads created.\n");
}

//----------------------------------------------------------------------
// ThreadTest3
// 	Create 10 threads, then print out thread status.
//----------------------------------------------------------------------

void
ThreadTest3()
{
    DEBUG('t', "Entering ThreadTest3\n");

    for (int i = 0; i < 10; i++) {
        char* name = new char[13];
        sprintf(name, "%dth thread", i);
        Thread *t = new Thread(name);
    }

    printf("10 threads created.\n");
    Thread::printTS();
}

//----------------------------------------------------------------------
// ThreadTest4
// 	Create threads with priority 5341.
//----------------------------------------------------------------------

void
ThreadTest4()
{
    DEBUG('t', "Entering ThreadTest4\n");

    Thread *t05 = new Thread("T0-5", 5);
    t05->Fork(SimpleThread, 0);
    Thread *t13 = new Thread("T1-3", 3);
    t13->Fork(SimpleThread, 1);
    Thread *t24 = new Thread("T2-4", 4);
    t24->Fork(SimpleThread, 2);
    Thread *t35 = new Thread("T3-5", 1);
    t35->Fork(SimpleThread, 3);
}

//----------------------------------------------------------------------
// ThreadTest5
// 	Create a thread with priority 5, which spawns another thread
//  with priority 4 hereafter.
//----------------------------------------------------------------------

void
ThreadTest5()
{
    DEBUG('t', "Entering ThreadTest4\n");

    Thread *t = new Thread("T0-5", 5);
    t->Fork(HighInLowThread, 0);
}

//----------------------------------------------------------------------
// ThreadTest
// 	Invoke a test routine.
//----------------------------------------------------------------------

void
ThreadTest()
{
    switch (testnum) {
    case 1:
	ThreadTest1();
	break;
    case 2:
    ThreadTest2();
    break;
    case 3:
    ThreadTest3();
    break;
    case 4:
    ThreadTest4();
    break;
    case 5:
    ThreadTest5();
    break;
    default:
	printf("No test specified.\n");
	break;
    }
}

