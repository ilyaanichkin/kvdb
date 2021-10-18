#pragma once

#include <cinttypes> // size_t
#include <boost/asio/io_service.hpp> // io_service
#include <boost/asio/ip/tcp.hpp> // acceptor
#include <boost/asio/strand.hpp> // strand
#include <memory> // shared_ptr
#include <array> // array

#include "../kvdb_data_models/kvdb_data_models.hpp"
#include "kvdb_server_stats.hpp"

namespace storage
{

class IStorage;

}

namespace network
{

class TcpConnection : public std::enable_shared_from_this< TcpConnection >
{
public:
    TcpConnection( boost::asio::io_service &io_service, storage::IStorage& strg, stats::IStats& stats );
    boost::asio::ip::tcp::socket& Socket();
    void Start();
    void HandleReadHeader( boost::system::error_code const& error, size_t bytes_transferred );
    void HandleReadBody( boost::system::error_code const& error, size_t bytes_transferred );
    void HandleWriteReply( boost::system::error_code const& error, size_t bytes_transferred );

private:
    boost::asio::io_service::strand mStrand;
    boost::asio::ip::tcp::socket mSocket;
    RequestHeader mHeader;
    size_t mOffset;
    std::vector< char > mBody;
    storage::IStorage& mStorage;
    stats::IStats& mStats;
};

class RequestProcessor
{
public:
    RequestProcessor();
};

class TcpListener
{
public:
    TcpListener( boost::asio::io_service& io_service, size_t port, storage::IStorage& strg, stats::IStats& stats );
    void Run( size_t threads );
    void StartAccept();
    void HandleAccept( boost::system::error_code const& ec );

private:
    boost::asio::io_service& mIoService;
    boost::asio::ip::tcp::acceptor mAcceptor;
    std::shared_ptr< TcpConnection > mConnection;
    storage::IStorage& mStorage;
    stats::IStats& mStats;
};

} // namespace network
