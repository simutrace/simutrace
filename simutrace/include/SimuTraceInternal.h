/*
 * Copyright 2015 (C) Karlsruhe Institute of Technology (KIT)
 * Marc Rittinghaus
 *
 *             _____ _                 __
 *            / ___/(_)___ ___  __  __/ /__________ _________
 *            \__ \/ / __ `__ \/ / / / __/ ___/ __ `/ ___/ _ \
 *           ___/ / / / / / / / /_/ / /_/ /  / /_/ / /__/  __/
 *          /____/_/_/ /_/ /_/\__,_/\__/_/   \__,_/\___/\___/
 *                         http://simutrace.org
 *
 * Simutrace Client Library (libsimutrace) is part of Simutrace.
 *
 * libsimutrace is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * libsimutrace is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libsimutrace. If not, see <http://www.gnu.org/licenses/>.
 */
/*! \file */
#pragma once
#ifndef SIMUTRACE_INTERNAL_H
#define SIMUTRACE_INTERNAL_H

#ifndef SIMUTRACE
#error "This header is for internal use only."
#endif

#include "SimuBase.h"
#include "SimuTrace.h"

namespace SimuTrace
{

    class SimutraceException :
        public Exception
    {
    private:
        static std::string _getErrorString()
        {
            ExceptionInformation info;
            StGetLastError(&info);

            return std::string(info.message);
        }

        static ExceptionClass _getErrorClass()
        {
            ExceptionInformation info;
            StGetLastError(&info);

            return info.type;
        }

        static int _getErrorCode()
        {
            return StGetLastError(nullptr);
        }
    public:
        SimutraceException(LOC_PARAM0) :
            Exception(LOC_ARG
                      _getErrorString(), _getErrorClass(), _getErrorCode()) {}

        ~SimutraceException() throw() {}
    };

}

#endif