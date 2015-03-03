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

#include "ChannelProvider.h"

#include "Exceptions.h"
#include "Utils.h"

#include "Channel.h"
#include "LocalChannel.h"
#include "SocketChannel.h"

namespace SimuTrace {
namespace ChannelProvider
{

    std::unique_ptr<Channel> createChannelFromSpecifier(bool isServer,
        const std::string& specifier)
    {
        // Define supported channel prefixes and corresponding constructors
        const int numPrefixes = 2;
        static struct {
            std::string prefix;
            std::unique_ptr<Channel> (*factoryMethod)(bool, const std::string&);
        } prefix[numPrefixes] = {
            { "local:", LocalChannel::factoryMethod },
            { "socket:", SocketChannel::factoryMethod }
        };


        // Check which type of channel the user has requested by finding a
        // matching channel prefix in the specifier.
        std::unique_ptr<Channel> channel;
        for (int i = 0; i < numPrefixes; i++) {

            if (specifier.compare(0, prefix[i].prefix.length(),
                                  prefix[i].prefix) == 0) {

                // We have identified the channel type. Cut away the prefix and
                // create the requested channel.
                std::string addr = specifier.substr(prefix[i].prefix.length(),
                                                    std::string::npos);

                channel = prefix[i].factoryMethod(isServer, addr);
                break;
            }

        }

        ThrowOnNull(channel, NotSupportedException);

        return channel;
    }

}
}