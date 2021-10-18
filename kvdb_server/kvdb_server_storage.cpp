#include "kvdb_server_storage.hpp"

#include <cinttypes>
#include <iostream>
#include "kvdb_server_st_m.hpp"
#include "kvdb_server_st_p.hpp"

namespace storage
{

IStorage::~IStorage()
{
}

std::unique_ptr< IStorage > InitializeStorage( size_t size, IStorage::Type type )
{
    switch( type )
    {
        case IStorage::tTemporal:
            return std::make_unique< TempStorage >();
        case IStorage::tPersistent:
            return std::make_unique< PersistentStorage >( size );
    }
    return nullptr;
}

} // namespace storage
