/*
 * Copyright 2014 (C) Karlsruhe Institute of Technology (KIT)
 * Marc Rittinghaus, Thorsten Groeninger
 * 
 * Simutrace Storage Server (storageserver) is part of Simutrace.
 *
 * storageserver is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * storageserver is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with storageserver. If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once
#ifndef WORKER_POOL_H
#define WORKER_POOL_H

#include "SimuStor.h"
#include "WorkQueue.h"

namespace SimuTrace
{

    class WorkItemBase;
    typedef Thread<WorkQueue&> WorkerThread;

    class WorkerPool
    {
    private:
        typedef ObjectReference<WorkerThread> WorkerReference;

    private:
        DISABLE_COPY(WorkerPool);

        mutable CriticalSection _lock;
        LogCategory _log;
        Environment _environment;
        WorkQueue _queue;

        std::vector<WorkerReference> _workers;

        void _freeWorkers();
        static int _workerMain(WorkerThread& thread);
    public:
        WorkerPool(uint32_t numWorkers, Environment& root);

        void close(bool dropQueue = false);

        void submitWork(std::unique_ptr<WorkItemBase>& item, 
                        WorkQueue::Priority priority = WorkQueue::Priority::Normal);

        void setPoolPriority(int priority);

        uint32_t getQueueLength() const;
        uint32_t getWorkerCount() const;

        bool tryProcessWorkItem();
    };

}

#endif