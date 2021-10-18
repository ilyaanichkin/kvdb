#pragma once

#include "kvdb_server_storage.hpp"

#include <cinttypes> // size_t
#include <string>
#include <string_view>
#include <map>
#include <shared_mutex>
#include <optional>

namespace storage
{

class TempStorage : public IStorage
{
public:
    TempStorage();
    IStorage::ErrorCode Insert( std::string_view key, std::string_view value ) override;
    IStorage::ErrorCode Update( std::string_view key, std::string_view value ) override;
    IStorage::ErrorCode Delete( std::string_view key ) override;
    std::optional< std::string > Get( std::string_view key ) override;
    size_t GetItemCount() override;

private:
    std::map< std::string, std::string, std::less<> > mMap;
    mutable std::shared_mutex mMutex;
};

} // namespace storage
