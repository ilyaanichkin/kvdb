#include "kvdb_server_network.hpp"
#include "kvdb_server_storage.hpp"

#include <iostream>
#include <thread>
#include <vector>
#include <string>
#include <boost/asio.hpp>

namespace network
{

TcpConnection::TcpConnection( boost::asio::io_service &io_service, storage::IStorage& strg, stats::IStats& stats )
    : mStrand( io_service )
    , mSocket( io_service )
    , mHeader( DecodedHeader( Opcode::opInvalid, 0, 0 ) )
    , mStorage( strg )
    , mStats( stats )
{
}

boost::asio::ip::tcp::socket& TcpConnection::Socket()
{
    return mSocket;
}

void TcpConnection::Start()
{
    mOffset = 0;
    boost::asio::async_read(
                mSocket,
                boost::asio::buffer( &mHeader, sizeof( mHeader ) ),
                boost::asio::transfer_exactly( sizeof( mHeader ) ),
                mStrand.wrap(
                    [ keep = this->shared_from_this(), this ]( boost::system::error_code const& ec, size_t bytes ){ HandleReadHeader( ec, bytes ); }
                    )
                );
}

void TcpConnection::HandleReadHeader( boost::system::error_code const& ec, size_t bytes )
{
    if( ec )
    {
        std::cerr << "ERROR " << ec << " while receiving header" << std::endl;
        return;
    }

    if( bytes + mOffset < sizeof( mHeader ) )
    {
        mOffset += bytes;
        boost::asio::async_read(
                    mSocket,
                    boost::asio::buffer( ( char* )( &mHeader ) + mOffset, sizeof( mHeader ) - mOffset ),
                    boost::asio::transfer_exactly( sizeof( mHeader ) - mOffset ),
                    mStrand.wrap(
                        [ keep = this->shared_from_this(), this ]( boost::system::error_code const& ec, size_t bytes ){ HandleReadHeader( ec, bytes ); }
                        )
                    );
        return;
    }

    if( bytes + mOffset > sizeof( mHeader ) ) // this should not happen, right?
    {
        std::cerr << "ERROR : message header received too long: " << bytes << " + " << mOffset << " offset" << std::endl;
        return;
    }

    DecodedHeader h{ mHeader };
    mBody.resize( h.mKeyLength + h.mValueLength + sizeof( RequestFooter ) );

    mOffset = 0;

    boost::asio::async_read(
                mSocket,
                boost::asio::buffer( mBody ),
                boost::asio::transfer_exactly( mBody.size() ),
                mStrand.wrap(
                    [ keep = this->shared_from_this(), this ]( boost::system::error_code const& ec, size_t bytes ){ HandleReadBody( ec, bytes ); }
                    )
                );
}

void TcpConnection::HandleReadBody( boost::system::error_code const& ec, size_t bytes )
{
    if( ec )
    {
        std::cerr << "ERROR " << ec << " while receiving body" << std::endl;
        return;
    }

    if( bytes + mOffset < mBody.size() )
    {
        mOffset += bytes;
        boost::asio::async_read(
                    mSocket,
                    boost::asio::buffer( mBody.data() + mOffset , mBody.size() - mOffset ),
                    boost::asio::transfer_exactly( mBody.size() - mOffset ),
                    mStrand.wrap(
                        [ keep = this->shared_from_this(), this ]( boost::system::error_code const& ec, size_t bytes ){ HandleReadBody( ec, bytes ); }
                        )
                    );
        return;
    }

    if( bytes + mOffset > mBody.size() ) // this should not happen, right?
    {
        std::cerr << "ERROR : message body received too long: " << bytes << " + " << mOffset << " offset" << std::endl;
        return;
    }

    if( mBody.size() < 8 )
    {
        std::cerr << "ERROR : message body received too short: " << mBody.size() << std::endl;
        return;
    }

    std::array< char, 8 > footer;
    char *tail = mBody.data() + mBody.size() - 8;
    for( size_t i = 0; i < 8; i++ )
        footer[ i ] =  tail[ i ];

    if( footer != RequestFooter::MAGIC )
    {
        std::cerr << "ERROR : message body tail corrupted" << std::endl;
        return;
    }

    DecodedHeader h{ mHeader };
    mOffset = 0;

    std::string reply;
    switch( h.mOpcode )
    {
        case Opcode::opInsert:
        {
            auto r = mStorage.Insert( std::string_view( mBody.data(), h.mKeyLength ), std::string_view( mBody.data() + h.mKeyLength, h.mValueLength ) );
            mStats.RegisterOperation( h.mOpcode, r == storage::IStorage::ecSuccess );
            switch( r )
            {
                case storage::IStorage::ecSuccess:
                    reply = "OK";
                    break;
                case storage::IStorage::ecKeyAlreadyExists:
                    reply = "ERROR: key to insert already exists";
                    break;
                default:
                    reply = "ERROR: unexpected operation result";
                    break;
            }
            break;
        }
        case Opcode::opUpdate:
        {
            auto r = mStorage.Update( std::string_view( mBody.data(), h.mKeyLength ), std::string_view( mBody.data() + h.mKeyLength, h.mValueLength ) );
            mStats.RegisterOperation( h.mOpcode, r == storage::IStorage::ecSuccess );
            switch( r )
            {
                case storage::IStorage::ecSuccess:
                    reply = "OK";
                    break;
                case storage::IStorage::ecKeyNotFound:
                    reply = "ERROR: key to update not found";
                    break;
                case storage::IStorage::ecValueNotChanged:
                    reply = "WARNING: value for key not changed";
                    break;
                default:
                    reply = "ERROR: unexpected operation result";
                    break;
            }
            break;
        }
        case Opcode::opDelete:
        {
            auto r = mStorage.Delete( std::string_view( mBody.data(), h.mKeyLength ) );
            mStats.RegisterOperation( h.mOpcode, r == storage::IStorage::ecSuccess );
            switch( r )
            {
                case storage::IStorage::ecSuccess:
                    reply = "OK";
                    break;
                case storage::IStorage::ecKeyNotFound:
                    reply = "ERROR: key to delete not found";
                    break;
                default:
                    reply = "ERROR: unexpected operation result";
                    break;
            }
            break;
        }
        case Opcode::opGet:
        {
            auto r = mStorage.Get( std::string_view( mBody.data(), h.mKeyLength ) );
            mStats.RegisterOperation( h.mOpcode, r.has_value() );
            if( r )
                reply = "Key is \"" + *r + "\"";
            else
                reply = "ERROR: key not found";
            break;
        }
        default:
            reply = "ERROR: unknown operation";
    }

    boost::asio::async_write(
                mSocket,
                boost::asio::buffer( reply ),
                mStrand.wrap(
                    [ keep = this->shared_from_this(), this ]( boost::system::error_code const& ec, size_t bytes ){ HandleWriteReply( ec, bytes ); }
                    )
                );
}

void TcpConnection::HandleWriteReply( boost::system::error_code const& ec, size_t bytes )
{
    // Just close connection for simplicity. Anyway, client supports only a single command per launch.
    if( !ec )
    {
        boost::system::error_code e;
        mSocket.shutdown( boost::asio::ip::tcp::socket::shutdown_both, e );
    }
}

TcpListener::TcpListener( boost::asio::io_service& io_service, size_t port, storage::IStorage& strg, stats::IStats& stats )
    : mIoService( io_service )
    , mAcceptor( mIoService )
    , mStorage( strg )
    , mStats( stats )
{
    boost::asio::ip::tcp::endpoint endpoint{ boost::asio::ip::tcp::v4(), static_cast< unsigned short >( port ) };
    mAcceptor.open( endpoint.protocol() );
    mAcceptor.set_option( boost::asio::ip::tcp::acceptor::reuse_address( true ) );
    mAcceptor.bind( endpoint );
    mAcceptor.listen();
    StartAccept();
}

void TcpListener::Run( size_t threads )
{
    std::vector< std::thread > thread_pool;
    for( size_t i = 0; i < threads; i++ )
        thread_pool.emplace_back( [&](){ mIoService.run(); } );
    for( std::thread &t : thread_pool )
        t.join();
}

void TcpListener::StartAccept()
{
    mConnection = std::make_shared< TcpConnection >( mIoService, mStorage, mStats );
    mAcceptor.async_accept(
                mConnection->Socket(),
                [&]( boost::system::error_code const& ec ){ HandleAccept( ec ); }
                );
}

void TcpListener::HandleAccept( boost::system::error_code const& ec )
{
    if( !ec )
        mConnection->Start();

    StartAccept();
}

void RunAsioServer( boost::asio::io_service& io_service, size_t port, size_t threads, storage::IStorage& strg, stats::IStats& stats )
{
    std::cout << "Start listening on TCP port " << port << " using " << threads << " threads..." << std::endl;
    TcpListener listener( io_service, port, strg, stats );
    listener.Run( threads );
}

} // namespace network
