#pragma once

#include "kvdb_server_storage.hpp"

#include <cinttypes> // size_t
#include <boost/interprocess/containers/string.hpp>
#include <string_view>

namespace std
{

template<>
struct less< boost::interprocess::string >
{
    template< typename S1, typename S2 >
    constexpr auto operator()( S1 && lhs, S2 && rhs ) const
    {
        return string_view( lhs.begin(), lhs.size() ) < string_view( rhs.begin(), rhs.size() );
    }
};

} // namespace std

#include <boost/filesystem.hpp>
#define USE_MMF
#if defined USE_MMF
#include <boost/interprocess/managed_mapped_file.hpp>
#elif defined _WIN32
#include <boost/interprocess/managed_windows_shared_memory.hpp>
#else
#include <boost/interprocess/managed_shared_memory.hpp>
#endif
#include <boost/interprocess/mem_algo/rbtree_best_fit.hpp>
#include <boost/interprocess/sync/null_mutex.hpp>
#include <boost/interprocess/indexes/iset_index.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/segment_manager.hpp>
#include <boost/interprocess/sync/named_sharable_mutex.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>

namespace storage
{

struct Item
{
    Item( std::string_view k, std::string_view v )
        : key( k.begin(), k.end() )
        , value( v.begin(), v.end() )
    {
    }
    boost::interprocess::string key;
    boost::interprocess::string value;
};

struct idx_key{};

#if defined USE_MMF
typedef boost::interprocess::basic_managed_mapped_file <
   char,
   boost::interprocess::rbtree_best_fit< boost::interprocess::null_mutex_family >,
   boost::interprocess::iset_index
> MemoryType;
#elif defined _WIN32
typedef boost::interprocess::basic_managed_windows_shared_memory <
   char,
   boost::interprocess::rbtree_best_fit< boost::interprocess::null_mutex_family >,
   boost::interprocess::iset_index
> MemoryType;
#else
typedef boost::interprocess::basic_managed_shared_memory <
   char,
   boost::interprocess::rbtree_best_fit< boost::interprocess::null_mutex_family >,
   boost::interprocess::iset_index
> MemoryType;
#endif

typedef boost::interprocess::allocator<
    Item,
    MemoryType::segment_manager
> ItemAllocator;

typedef struct boost::multi_index_container<
    Item,
    boost::multi_index::indexed_by<
        boost::multi_index::ordered_unique<
            boost::multi_index::tag< idx_key >, BOOST_MULTI_INDEX_MEMBER( Item, boost::interprocess::string, key )
        >
    >,
    ItemAllocator
> ItemMap;

class PersistentStorage : public IStorage
{
public:
    PersistentStorage( size_t size );
    IStorage::ErrorCode Insert( std::string_view key, std::string_view value ) override;
    IStorage::ErrorCode Update( std::string_view key, std::string_view value ) override;
    IStorage::ErrorCode Delete( std::string_view key ) override;
    std::optional< std::string > Get( std::string_view key ) override;
    size_t GetItemCount() override;

private:
    std::string mPath;
    MemoryType mBuffer;
    ItemMap& mMap;
    boost::interprocess::named_sharable_mutex mMutex;
};

} // namespace storage
