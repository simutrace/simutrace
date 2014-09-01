/*
 * Copyright 2014 (C) Karlsruhe Institute of Technology (KIT)
 * Marc Rittinghaus, Thorsten Groeninger
 *
 * Simutrace Base Library (libsimubase) is part of Simutrace.
 *
 * libsimubase is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * libsimubase is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libsimubase. If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once
#ifndef THREAD_BASE_H
#define THREAD_BASE_H

#include "SimuPlatform.h"
#include "SimuBaseTypes.h"

#include "SafeHandle.h"

#include <signal.h>
#include <setjmp.h>

#ifdef WIN32
DWORD WINAPI ThreadStart(LPVOID param);
#else
#endif

namespace SimuTrace
{

    typedef enum _ThreadState {
        TsIdle = 0,
        TsStarting,
        TsRunning,
        TsStopping,
        TsFinished
    } ThreadState;

#ifdef WIN32
#else
    typedef struct _SignalJumpBuffer {
        sigjmp_buf signalret;
    } SignalJumpBuffer;
#endif

    namespace System {
        uint32_t getNumLogicalProcessors();
    }

    class ThreadBase 
    {
    private:
        DISABLE_COPY(ThreadBase);

        ThreadState _state;
        int _retVal;
        int _priority;
    #ifdef WIN32
        SafeHandle _thread;
        DWORD _threadId;

        friend DWORD WINAPI ThreadStart(LPVOID);
    #else
        SignalJumpBuffer* _signalJmp;
        pthread_t _threadId;

        void _prepareSigBusHandling();

        friend void* ThreadStart(void*);
    #endif

        static __thread ThreadBase* _currentThread;
    protected:
        virtual int _run() = 0;
        virtual void _onFinalize() = 0;

    public:
        ThreadBase();
        virtual ~ThreadBase();

        // Starts the thread and calls run() in the context of the new thread
        void start();

    #ifdef WIN32
    #else
        // Sets/Gets the signal return point
        void setSignalJmpBuffer(SignalJumpBuffer* jmp);
        SignalJumpBuffer* getSignalJmpBuffer() const;
    #endif

        // Notifies the thread to end
        void stop(bool force = false);
        bool shouldStop() const;

        // Waits for the thread to finish its execution
        void waitForThread() const;

        // Get or set the priority of the thread
        void setPriority(int priority);
        int getPriority() const;

        // Checks if the thread has finished execution
        bool hasFinished() const;
        bool isRunning() const;

        // Checks if this is the executing thread
        bool isExecutingThread() const;

        // Returns the thread id
        unsigned long getId() const;

        // The current thread will sleep for the specified milliseconds
        static void sleep(uint32_t ms);

        // Returns the current thread id
        // Note: On Linux this call return the pthread id, not the thread's id
        // in the system. Use getCurrentSystemThreadId() instead.
        static unsigned long getCurrentThreadId();

        // Returns the current thread id used in the OS
        static unsigned long getCurrentSystemThreadId();

        static ThreadBase* getCurrentThread();

        // Returns the current process id
        static unsigned long getCurrentProcessId();

        // Waits for any thread
        static void waitForThread(unsigned long threadId);
    };
}

#endif