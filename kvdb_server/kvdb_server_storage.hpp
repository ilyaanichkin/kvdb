#pragma once

#include <cinttypes> // size_t
#include <string>
#include <string_view>
#include <map>
#include <shared_mutex>
#include <optional>

namespace storage
{

class IStorage
{
public:
    enum Type
    {
        tTemporal,
        tPersistent
    };
    enum ErrorCode
    {
        ecSuccess,
        ecKeyNotFound,
        ecKeyAlreadyExists,
        ecValueNotChanged
    };
    virtual ~IStorage() = 0;
    virtual ErrorCode Insert( std::string_view key, std::string_view value ) = 0;
    virtual ErrorCode Update( std::string_view key, std::string_view value ) = 0;
    virtual ErrorCode Delete( std::string_view key ) = 0;
    virtual std::optional< std::string > Get( std::string_view key ) = 0;
    virtual size_t GetItemCount() = 0;
};

std::unique_ptr< IStorage > InitializeStorage( size_t size, IStorage::Type type );

} // namespace storage
