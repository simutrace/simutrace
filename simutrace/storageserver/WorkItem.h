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
#ifndef WORK_ITEM_H
#define WORK_ITEM_H

#include "SimuStor.h"
#include "WorkItemBase.h"

namespace SimuTrace
{

    template<typename T>
    class WorkItem :
        public WorkItemBase
    {
    public:
        typedef void (*WorkMain)(WorkItem<T>&, T&);

    private:
        const WorkMain _main;
        T _argument;

    public:
        WorkItem(WorkMain main, T& argument) :
            _main(main),
            _argument(argument) { }
        virtual ~WorkItem() override { }

        virtual void execute() override { _main(*this, _argument); }

        T& getArgument() { return _argument; }
    };

}

#endif