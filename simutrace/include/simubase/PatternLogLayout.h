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
#ifndef _PATTERN_LOG_LAYOUT_
#define _PATTERN_LOG_LAYOUT_

#include "SimuPlatform.h"
#include "SimuBaseTypes.h"

#include "LogLayout.h"

namespace SimuTrace
{

    class PatternLogLayout :
        public LogLayout
    {
    public:
        class PatternComponent
        {
        public:
            inline virtual ~PatternComponent() {};

            virtual void append(std::ostringstream& out,
                                const LogEvent& event) = 0;
        };

    private:
        DISABLE_COPY(PatternLogLayout);

        const std::string _pattern;
        std::vector<std::shared_ptr<PatternComponent>> _components;

        void _freePattern();
        void _updatePattern();
    public:
        PatternLogLayout(const std::string& pattern);
        virtual ~PatternLogLayout();

        const std::string& getPattern() const;

        virtual std::string format(const LogEvent& event) override;
    };

}

#endif