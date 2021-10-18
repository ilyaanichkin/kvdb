#include "kvdb_server_stats.hpp"

#include <boost/asio/io_service.hpp>
#include <iostream>

#include "kvdb_server_storage.hpp"

namespace stats
{

IStats::~IStats()
{
}

Stats::Stats( boost::asio::io_service& io_service, storage::IStorage& storage, size_t interval_seconds )
    : mInterval( interval_seconds )
    , mTimer( io_service, mInterval )
    , mStorage( storage )
{
    for( size_t i = 0; i < static_cast< size_t >( Opcode::op__MaxCount ); i++ )
    {
        mSuccess[ i ].store( 0 );
        mFailure[ i ].store( 0 );
    }
}

void Stats::RegisterOperation( Opcode type, bool success )
{
    if( success )
        mSuccess[ static_cast< size_t >( type ) ].fetch_add( 1, std::memory_order_release );
    else
        mFailure[ static_cast< size_t >( type ) ].fetch_add( 1, std::memory_order_release );
}

void Stats::TimedReporting( boost::system::error_code const& ec )
{
    if( ec )
        return;

    mTimer.expires_from_now( mInterval );

    std::cerr << "Records: " << mStorage.GetItemCount()
              << ", Succeeded/Failed operations:";
    for( size_t i = 0; i < static_cast< size_t >( Opcode::op__MaxCount ); i++ )
    {
        std::cerr << " " << mNames[ i ] << ": "
                  << mSuccess[ i ].load( std::memory_order_consume ) << "/"
                  << mFailure[ i ].load( std::memory_order_consume ) << ",";
    }
    std::cerr << std::endl;

    mTimer.async_wait( [ this ]( boost::system::error_code const& ec ){ TimedReporting( ec ); } );
}

void Stats::Launch()
{
    mTimer.expires_from_now( boost::posix_time::seconds( 1 ) );
    mTimer.async_wait( [ this ]( boost::system::error_code const& ec ){ TimedReporting( ec ); } );
}

} // namespace stats
