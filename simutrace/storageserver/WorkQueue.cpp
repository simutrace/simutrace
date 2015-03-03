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
#include "SimuStor.h"

#include "WorkQueue.h"

#include "WorkItemBase.h"
#include "WorkerPool.h"

namespace SimuTrace
{

    WorkQueue::WorkQueue(WorkerPool& pool) :
        _pool(pool),
        _blocked(false)
    {

    }

    WorkQueue::~WorkQueue()
    {
        assert(_queue[Priority::Low].empty());
        assert(_queue[Priority::Normal].empty());
        assert(_queue[Priority::High].empty());
    }

    void WorkQueue::enqueueWork(std::unique_ptr<WorkItemBase>& item,
                                Priority priority)
    {
        LockScope(_lock);
        ThrowOn(_blocked, InvalidOperationException);
        ThrowOn(priority > Priority::Max, ArgumentOutOfBoundsException);

        _queue[priority].push(std::move(item));
        wakeUpWorkers();
    }

    std::unique_ptr<WorkItemBase> WorkQueue::dequeueWork()
    {
        LockScope(_lock);

        for (int p = Priority::Max; p >= 0; --p) {
            auto& queue = _queue[p];

            if (!queue.empty()) {
                auto item = std::move(queue.front());
                queue.pop();

                return item;
            }
        }

        // All queues are empty
        if (_blocked) {
            _emptyEvent.signal();
        }

        return nullptr;
    }

    void WorkQueue::clear()
    {
        LockScope(_lock);

        // Clear the queue by swapping it with an empty one
        std::queue<std::unique_ptr<WorkItemBase>> empty[Priority::Max + 1];
        std::swap(_queue, empty);
    }

    void WorkQueue::block()
    {
        LockScope(_lock);

        _blocked = true;
        if (isEmpty()) {
            _emptyEvent.signal();
        }
    }

    void WorkQueue::wait()
    {
        _workEvent.wait();
    }

    void WorkQueue::waitEmpty()
    {
        _emptyEvent.wait();
    }

    void WorkQueue::wakeUpWorkers()
    {
        // BUG! This will only wake up one of the workers! Will be fixed
        // with the use of conditional variable!
        _workEvent.signal();
    }

    bool WorkQueue::isEmpty() const
    {
        LockScope(_lock);

        for (int p = Priority::Max; p >= 0; --p) {
            if (!_queue[p].empty()) {
                return false;
            }
        }

        return true;
    }

    uint32_t WorkQueue::getLength() const
    {
        LockScope(_lock);

        uint32_t num = 0;
        for (int p = Priority::Max; p >= 0; --p) {
            num += static_cast<uint32_t>(_queue[p].size());
        }

        return num;
    }

    WorkerPool& WorkQueue::getPool() const
    {
        return _pool;
    }

}