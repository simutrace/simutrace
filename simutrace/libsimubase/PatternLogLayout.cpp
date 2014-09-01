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
#include "SimuPlatform.h"
#include "SimuBaseTypes.h"

#include "PatternLogLayout.h"

#include "Clock.h"
#include "Exceptions.h"
#include "Utils.h"

#include "LogCategory.h"
#include "LogLayout.h"
#include "LogEvent.h"

namespace SimuTrace
{

    //
    // Definition of various pattern layout components
    //

    class StringLiteralComponent :
        public PatternLogLayout::PatternComponent
    {
    private:
        const std::string _literal;

    public:
        StringLiteralComponent(const std::string& literal) :
            _literal(literal) { }

        virtual void append(std::ostringstream& out, 
                            const LogEvent& event) override
        {
            out << _literal;
        }
    };

    class CategoryNameComponent :
        public PatternLogLayout::PatternComponent
    {
    public:
        virtual void append(std::ostringstream& out, 
                            const LogEvent& event) override
        {
            out << event.getCategory().getName();
        }
    };

    class NewLineSpaceComponent :
        public PatternLogLayout::PatternComponent
    {
    private:
        bool _constructed;
        std::string _space;

    public:
        NewLineSpaceComponent() :
            _constructed(false),
            _space() {}

        virtual void append(std::ostringstream& out, 
                            const LogEvent& event) override
        {
            if (!_constructed) {
                _space.assign(out.tellp(), ' ');
                _constructed = true;
            }
        }

        const std::string& getSpace() const { return _space; }
    };

    class MessageComponent :
        public PatternLogLayout::PatternComponent
    {
    private:
        const std::shared_ptr<NewLineSpaceComponent> _space;

    public:
        MessageComponent(const std::shared_ptr<NewLineSpaceComponent>& space) :
            _space(space) {}

        virtual void append(std::ostringstream& out, 
                            const LogEvent& event) override
        {
            if (_space != nullptr) {
                std::string msg = event.getMessage();
                
                std::string::size_type pos;
                while ((pos = msg.find('\n')) != std::string::npos) {
                    out << msg.substr(0, pos + 1)
                        << _space->getSpace();

                    msg = msg.substr(pos + 1);
                }

                out << msg;
            } else {
                out << event.getMessage();
            }
        }
    };

    class PriorityComponent :
        public PatternLogLayout::PatternComponent
    {
    public:
        virtual void append(std::ostringstream& out, 
                            const LogEvent& event) override
        {
            out << LogPriority::getPriorityName(event.getPriority());
        }
    };

    class NewLineComponent :
        public PatternLogLayout::PatternComponent
    {
    private:
        const std::shared_ptr<NewLineSpaceComponent> _space;

    public:
        NewLineComponent(std::shared_ptr<NewLineSpaceComponent>& space) :
            _space(space) {}

        virtual void append(std::ostringstream& out, 
                            const LogEvent& event) override
        {
            out << "\n";

            if (_space != nullptr) {
                out << _space->getSpace();
            }
        }

    };

    class TimestampComponent :
        public PatternLogLayout::PatternComponent
    {
    private:
        std::string _format;

    public:
        TimestampComponent(const std::string& format) :
            _format(format) { }

        virtual void append(std::ostringstream& out, 
                            const LogEvent& event) override
        {
            out << Clock::formatTime(_format, event.getTimestamp());
        }
    };


    //
    // Definition of the PatternLogLayout class
    //

    PatternLogLayout::PatternLogLayout(const std::string& pattern) :
        LogLayout(),
        _pattern(pattern),
        _components()
    {
        _updatePattern();
    }

    PatternLogLayout::~PatternLogLayout()
    {
        _freePattern();
    }

    void PatternLogLayout::_freePattern()
    {
        _components.clear();
    }

    void PatternLogLayout::_updatePattern()
    {
        _freePattern();

        try {
            // Parse the pattern string and add the necessary components to
            // the chain. This chain will later be traversed to build the
            // formatted log string.

            const char* pattern = _pattern.c_str();
            uint32_t size = static_cast<uint32_t>(_pattern.size());
            uint32_t i = 0;

            std::shared_ptr<NewLineSpaceComponent> newLineSpace;
            std::shared_ptr<PatternComponent> cmp;
            std::string literal;
            while (i < size) {
                char chr = pattern[i++];

                // If the current character is a control character, we need to
                // further check what type of component we should create.
                // Otherwise, we add the character to the literal string from
                // which we create a string component when a new valid
                // component is detected and at the end.

                if (chr == '%') {
                    ThrowOn(i >= size, Exception, "Invalid pattern ending.");
                    chr = pattern[i++];

                    // Extract postfix if specified
                    std::string postfix = "";
                    if (i < size) {
                        char chr2 = pattern[i];

                        if (chr2 == '{') {
                            bool foundEnd = false;

                            i++;
                            while (i < size) {
                                char chr2 = pattern[i++];
                                if (chr2 != '}') {
                                    postfix += chr2;
                                } else {
                                    foundEnd = true;
                                    break;
                                }
                            }

                            ThrowOn(!foundEnd, Exception,
                                    "Missing end of postfix.");
                        }
                    }

                    // Process pattern code
                    switch (chr)
                    {
                        case '%': {
                            literal += chr;
                            break;
                        }

                        case 'c': {
                            cmp = std::make_shared<CategoryNameComponent>();
                            break;
                        }

                        case 'd': {
                            cmp = std::make_shared<TimestampComponent>(postfix);
                            break;
                        }
                        
                        case 'm': {
                            cmp = std::make_shared<MessageComponent>(newLineSpace);
                            break;
                        }

                        case 'n': {
                            literal += '\n';
                            break;
                        }

                        case 'p': {
                            cmp = std::make_shared<PriorityComponent>();
                            break;
                        }

                        case 's': {
                            newLineSpace = 
                                std::make_shared<NewLineSpaceComponent>();
                            cmp = newLineSpace;
                            break;
                        }

                        default: {
                            Throw(Exception, stringFormat(
                                  "Invalid pattern component '%c'.", chr));

                            break;
                        }
                    }

                } else {
                    if ((chr == '\n') && (newLineSpace == nullptr)) {
                        cmp = std::make_shared<NewLineComponent>(newLineSpace);
                    } else {
                        literal += chr;
                    }
                }

                if (cmp != nullptr) {

                    // Add any pending string literal
                    if (!literal.empty()) {
                        _components.push_back(
                            std::make_shared<StringLiteralComponent>(literal));

                        literal = "";
                    }

                    _components.push_back(cmp);
                    cmp = nullptr;
                }
            }

            // Add any pending string literal
            if (!literal.empty()) {
                _components.push_back(
                    std::make_shared<StringLiteralComponent>(literal));
            }

        } catch (...) {
            _freePattern();

            throw;
        }
    }

    const std::string& PatternLogLayout::getPattern() const
    {
        return _pattern;
    }

    std::string PatternLogLayout::format(const LogEvent& event)
    {
        std::ostringstream message;

        for (auto i = 0; i < _components.size(); ++i) {
            _components[i]->append(message, event);
        }

        return message.str();
    }

}