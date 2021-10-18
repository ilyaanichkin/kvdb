#pragma once

#include <cinttypes> // size_t
#include <string_view>
#include <array>
#include <atomic>
#include <boost/date_time/posix_time/posix_time_duration.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/io_service.hpp>
#include "../kvdb_data_models/kvdb_data_models.hpp"

constexpr std::string_view operator ""_sv( const char* str, std::size_t len ) noexcept
{
    return std::string_view( str, len );
}

namespace storage
{

class IStorage;

} // namespace storage

namespace stats
{

class IStats
{
public:
    virtual ~IStats();
    virtual void RegisterOperation( Opcode type, bool success ) = 0;
};

class Stats : public IStats
{
public:
    static constexpr std::array< std::string_view, 4 > mNames{ "Insert"_sv, "Update"_sv, "Delete"_sv, "Get"_sv };

    Stats( boost::asio::io_service& io_service, storage::IStorage& storage, size_t interval_seconds );
    void RegisterOperation( Opcode type, bool success ) override;
    void TimedReporting( boost::system::error_code const& ec );
    void Launch();

private:
    std::array< std::atomic< size_t >, static_cast< size_t >( Opcode::op__MaxCount ) > mSuccess;
    std::array< std::atomic< size_t >, static_cast< size_t >( Opcode::op__MaxCount ) > mFailure;

    boost::posix_time::seconds mInterval;
    boost::asio::deadline_timer mTimer;

    storage::IStorage& mStorage;
};

} // namespace stats
