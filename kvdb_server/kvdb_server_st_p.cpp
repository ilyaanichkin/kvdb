#include "kvdb_server_st_p.hpp"

#include <iostream>
#include <cinttypes>
#include <boost/interprocess/sync/sharable_lock.hpp>

namespace storage
{

struct ChangeValue
{
    ChangeValue( std::string_view value )
        : mValue( value )
    {
    }
    void operator()( Item& item )
    {
        item.value.assign( mValue.begin(), mValue.end() );
    }

private:
    std::string mValue;
};

PersistentStorage::PersistentStorage( size_t size )
    : mPath{ ( boost::filesystem::current_path() / "storage.bin" ).string() }
    , mBuffer{ boost::interprocess::open_or_create, "storage.bin", size }
    , mMap{ *mBuffer.find_or_construct< ItemMap >( "Map" )( ItemMap::ctor_args_list(), mBuffer.get_segment_manager() ) }
    , mMutex{ boost::interprocess::open_or_create, "storage.bin.mutex" }
{
    std::cout << "Persistent storage of size " << size << " bytes created..." << std::endl;
}

IStorage::ErrorCode PersistentStorage::Insert( std::string_view key, std::string_view value )
{
    boost::interprocess::scoped_lock< boost::interprocess::named_sharable_mutex > lock( mMutex );
    auto found = mMap.find( key );
    if( found != mMap.end() )
        return ecKeyAlreadyExists;
    mMap.emplace( key, value );
    return ecSuccess;
}

IStorage::ErrorCode PersistentStorage::Update( std::string_view key, std::string_view value )
{
    boost::interprocess::scoped_lock< boost::interprocess::named_sharable_mutex > lock( mMutex );
    auto found = mMap.find( key );
    if( found == mMap.end() )
        return ecKeyNotFound;
    if( std::string_view( found->value.begin(), found->value.size() ) == value )
        return ecValueNotChanged;
    mMap.modify( found, ChangeValue( value ) );
    return ecSuccess;
}

IStorage::ErrorCode PersistentStorage::Delete( std::string_view key )
{
    boost::interprocess::scoped_lock< boost::interprocess::named_sharable_mutex > lock( mMutex );
    if( mMap.erase( boost::interprocess::string( key.begin(), key.end() ) ) == 0 )
        return ecKeyNotFound;
    return ecSuccess;
}

std::optional< std::string > PersistentStorage::Get( std::string_view key )
{
    boost::interprocess::sharable_lock< boost::interprocess::named_sharable_mutex > lock( mMutex );
    auto found = mMap.find( key );
    if( found == mMap.end() )
        return {};
    return std::string( found->value.begin(), found->value.end() );
}

size_t PersistentStorage::GetItemCount()
{
    boost::interprocess::sharable_lock< boost::interprocess::named_sharable_mutex > lock( mMutex );
    return mMap.size();
}

} // namespace storage
