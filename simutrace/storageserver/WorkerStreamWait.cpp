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

#include "WorkerStreamWait.h"

#include "WorkerPool.h"

namespace SimuTrace
{

    WorkerStreamWait::WorkerStreamWait() :
        StreamWait()
    {
    
    }

    bool WorkerStreamWait::wait()
    {
        WorkerThread* worker = dynamic_cast<WorkerThread*>(
            ThreadBase::getCurrentThread());

        if (worker != nullptr) {
            assert(worker->isRunning());

            WorkQueue& queue = worker->getArgument();
            WorkerPool& pool = queue.getPool();

            // Before we check the work queue, we see if our wait has
            // already been satisfied. If not, we check the work queue for 
            // pending work items. If the work queue is empty, we break out of
            // the loop because all jobs, which the caller can possibly have 
            // submitted, are either finished or are currently being processed
            // by other worker threads. The wait should thus be satisfied soon.
            while ((this->getCount() > 0) && pool.tryProcessWorkItem()) { }
        }

        return this->StreamWait::wait();
    }

}