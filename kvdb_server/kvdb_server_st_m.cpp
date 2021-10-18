#include "kvdb_server_st_m.hpp"

#include <iostream>
#include <cinttypes>

namespace storage
{

TempStorage::TempStorage()
{
    std::cout << "Temporal storage created..." << std::endl;
}

IStorage::ErrorCode TempStorage::Insert( std::string_view key, std::string_view value )
{
    std::lock_guard< std::shared_mutex > lock( mMutex );
    auto found = mMap.find( key );
    if( found != mMap.end() )
        return ecKeyAlreadyExists;
    mMap.emplace( key, value );
    return ecSuccess;
}

IStorage::ErrorCode TempStorage::Update( std::string_view key, std::string_view value )
{
    std::lock_guard< std::shared_mutex > lock( mMutex );
    auto found = mMap.find( key );
    if( found == mMap.end() )
        return ecKeyNotFound;
    if( found->second == value )
        return ecValueNotChanged;
    found->second = value;
    return ecSuccess;
}

IStorage::ErrorCode TempStorage::Delete( std::string_view key )
{
    std::lock_guard< std::shared_mutex > lock( mMutex );
    if( mMap.erase( std::string( key ) ) == 0 )
        return ecKeyNotFound;
    return ecSuccess;
}

std::optional< std::string > TempStorage::Get( std::string_view key )
{
    std::shared_lock< std::shared_mutex > lock( mMutex );
    auto found = mMap.find( key );
    if( found == mMap.end() )
        return {};
    return found->second;
}

size_t TempStorage::GetItemCount()
{
    std::shared_lock< std::shared_mutex > lock( mMutex );
    return mMap.size();
}

} // namespace storage
