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

#include "WorkerPool.h"

#include "WorkItemBase.h"
#include "WorkQueue.h"

namespace SimuTrace
{

    WorkerPool::WorkerPool(uint32_t numWorkers, Environment& root) :
        _log("[Worker]", root.log),
        _environment(root),
        _queue(*this)
    {
        _environment.log = &_log;

        if (numWorkers == 0) {
            numWorkers = System::getNumLogicalProcessors();
        }

        try {

            for (uint32_t i = 0; i < numWorkers; i++) {
                WorkerReference thread(new WorkerThread(_workerMain, _queue),
                                       [](WorkerThread* instance) {
                    assert(!instance->isRunning());
                    unsigned long id = instance->getId();

                    delete instance;

                    LogDebug("Worker thread %d released.", id);
                });

                WorkerThread* pthread = thread.get();
                _workers.push_back(std::move(thread));

                pthread->start();
            }

        } catch (...) {
            close(true);

            throw;
        }
    }

    int WorkerPool::_workerMain(WorkerThread& thread)
    {
        WorkQueue& queue = thread.getArgument();
        WorkerPool& pool = queue.getPool();
        Environment* env = &pool._environment;
        Environment::set(env);

        while (!thread.shouldStop()) {
            queue.wait();

            while (pool.tryProcessWorkItem() && !thread.shouldStop()) { }
        }

        return 0;
    }

    bool WorkerPool::tryProcessWorkItem()
    {
        std::unique_ptr<WorkItemBase> item = _queue.dequeueWork();
        if (item == nullptr) {
            return false;
        }

        try {
            // Initializing the swapper with the worker pool environment.
            SwapEnvironment(&_environment);

            item->execute();
        } catch (const std::exception& e) {

            LogError("<worker: %d> Failed to process work item. "
                     "Exception: '%s'.",
                     ThreadBase::getCurrentSystemThreadId(),
                     e.what());

            // We do not re-throw the exception here. We expect the initiator
            // of the work to count with dropped work items.
        }

        return true;
    }

    void WorkerPool::close(bool dropQueue)
    {
        // Prevent new work items from entering the pool.
        _queue.block();

        if (dropQueue) {
            _queue.clear();
        } else {
            _queue.waitEmpty();
        }

        assert(_queue.isEmpty());

        // At this point, the queue is empty. Some workers might still process
        // work, others may sleep. We therefore, wake all workers up so they
        // recognize that they should stop.
        for (int i = 0; i < _workers.size(); i++) {
            _workers[i]->stop();
        }

        _queue.wakeUpWorkers();

        for (int i = 0; i < _workers.size(); i++) {
            _workers[i]->waitForThread();

            assert(_workers[i]->hasFinished());
        }

    }

    void WorkerPool::submitWork(std::unique_ptr<WorkItemBase>& item,
                                WorkQueue::Priority priority)
    {
        _queue.enqueueWork(item, priority);
    }

    void WorkerPool::setPoolPriority(int priority)
    {
        for (auto i = 0; i < _workers.size(); ++i) {
            _workers[i]->setPriority(priority);
        }
    }

    uint32_t WorkerPool::getQueueLength() const
    {
        return _queue.getLength();
    }

    uint32_t WorkerPool::getWorkerCount() const
    {
        return static_cast<uint32_t>(_workers.size());
    }

}
