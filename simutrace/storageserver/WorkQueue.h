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
#ifndef WORK_QUEUE_H
#define WORK_QUEUE_H

#include "SimuStor.h"

namespace SimuTrace
{

    class WorkItemBase;
    class WorkerPool;

    class WorkQueue
    {
    public:
        enum Priority {
            Low     = 0x00,
            Normal  = 0x01,
            High    = 0x02,

            Max     = High
         };
    private:
        DISABLE_COPY(WorkQueue);

        WorkerPool& _pool;

        mutable CriticalSection _lock;
        std::queue<std::unique_ptr<WorkItemBase>> _queue[Priority::Max + 1];

        Event _workEvent;
        Event _emptyEvent;

        bool _blocked;
    public:
        WorkQueue(WorkerPool& pool);
        ~WorkQueue();

        void enqueueWork(std::unique_ptr<WorkItemBase>& item,
                         Priority priority = Priority::Normal);
        std::unique_ptr<WorkItemBase> dequeueWork();

        void clear();

        void block();

        void wait();

        // Wait for the queue to be empty. Does only work in BLOCKED state!
        void waitEmpty();
        void wakeUpWorkers();

        bool isEmpty() const;
        uint32_t getLength() const;

        WorkerPool& getPool() const;
    };

}

#endif