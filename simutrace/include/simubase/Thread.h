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
#ifndef THREAD_H
#define THREAD_H

#include "SimuPlatform.h"
#include "ThreadBase.h"

namespace SimuTrace
{

    template<typename T>
    class Thread :
        public ThreadBase
    {
    public:
        typedef int (*ThreadMain)(Thread<T>&);

    private:
        DISABLE_COPY(Thread<T>);

        const ThreadMain _main;
        T _arg;

        virtual int _run() override { return _main(*this); }
        virtual void _onFinalize() override { }

    public:
        Thread(ThreadMain main, T& arg) : 
            _main(main),
            _arg(arg) { }
        Thread(ThreadMain main) :
            _main(main) { }
        virtual ~Thread() { }

        T& getArgument() { return _arg; }
        void setArgument(T& arg) { _arg = arg; }
    };

}

#endif