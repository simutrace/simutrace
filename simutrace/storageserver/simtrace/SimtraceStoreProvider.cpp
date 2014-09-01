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

#include "SimtraceStoreProvider.h"

#include "../ServerStore.h"
#include "../ServerSession.h"
#include "../StorageServer.h"
#include "Simtrace3Store.h"
#include "Simtrace3Format.h"

#include "FileHeader.h"

namespace SimuTrace {
namespace Simtrace {
namespace SimtraceStoreProvider
{

    std::unique_ptr<ServerStore> factoryMethod(StoreId id, 
                                               const std::string& path, 
                                               bool alwaysCreate)
    {
        uint16_t majorVersion = SIMTRACE_DEFAULT_VERSION;
        std::ifstream file(path.c_str(), std::ifstream::binary);

        if (file) {
            if (alwaysCreate) {
                file.close();

                LogInfo("Deleting existing store '%s'.", path.c_str());

                // We should always create a new store, so delete the existing
                File::remove(path);
            } else {
                SimtraceMasterHeader header;

                file.read((char*)(&header), sizeof(SimtraceMasterHeader));
                ThrowOn(!file || !isSimtraceStore(header), Exception, 
                        "Failed to read store master header."); 

                majorVersion = header.majorVersion;

                file.close();
            }
        }

        LogDebug("Trying to open Simtrace store '%s' using version %d.", 
                 path.c_str(), majorVersion);

        // ! ADD SUPPORT FOR NEW SIMTRACE STORE VERSIONS HERE !

        std::unique_ptr<ServerStore> store;
        switch (majorVersion) 
        {
            case SIMTRACE_VERSION3: {
                store = std::unique_ptr<Simtrace3Store>(
                    new Simtrace3Store(id, path, alwaysCreate)); 
                break;
            }

            default: {
                Throw(NotSupportedException);
                break;
            }
        }

        return store;
    }

    std::string makePath(const std::string& path)
    {
        std::string base, fpath;
        if (Configuration::tryGet("store.simtrace.root", base)) {
            fpath = Directory::makePath(&base, &path);
        } else {
            fpath = StorageServer::makePath(nullptr, &path);
        }

        return fpath;
    }

    void enumerationMethod(const std::string& path, 
                           std::vector<std::string>& out)
    {
        Directory directory(path, std::string(".sim"));

        directory.enumerateFiles(out);
    }

}
}
}