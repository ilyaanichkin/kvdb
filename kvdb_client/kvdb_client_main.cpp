#include <iostream>
#include <string>
#include <cctype> // toupper
#include <memory.h> // memcpy
#include <boost/asio.hpp>
#include "../kvdb_data_models/kvdb_data_models.hpp"

void PrintUsage()
{
    std::cerr << "Usage: kvdb_client <host>:<port> <command> <key> [<value>]" << std::endl
              << "where" << std::endl
              << "<host>:<port> is an address of a running kvdb_server, for example 127.0.0.1:10223" << std::endl
              << "<command> is one of these: INSERT, UPDATE, DELETE, GET" << std::endl
              << "<key> is a string with length up to 1024 (1k)" << std::endl
              << "<value> is a string with length up to 1048576 (1M)" << std::endl
              << "Use \"...\" to set key or value with spaces or an empty value" << std::endl
                 ;
}

int main( int argc, char **argv )
{
    try
    {
        if( argc < 4 )
        {
            PrintUsage();
            return 1;
        }

        // Host:Port
        std::string address{ argv[1] };
        size_t p = address.find( ':' );
        if( p == std::string::npos )
        {
            std::cerr << "Error: service address must be <host>:<port>" << std::endl;
            PrintUsage();
            return 1;
        }
        std::string host = address.substr( 0, p );
        std::string port = address.substr( p + 1 );

        // Command
        std::string command{ argv[2] };
        std::transform( command.begin(), command.end(), command.begin(), []( unsigned char c ){ return std::toupper( c ); } );
        if( command != "INSERT" && command != "UPDATE" && command != "DELETE" && command != "GET" )
        {
            std::cerr << "Error: <command> must be one of these: INSERT, UPDATE, DELETE, GET" << std::endl;
            PrintUsage();
            return 1;
        }
        while( command.size() < 8 )
            command.push_back( ' ' );

        // Ensure "Value" is present if needed
        if( ( command == "INSERT" || command == "UPDATE" ) && argc < 5 )
        {
            std::cerr << "Error: <value> is required for INSERT and UPDATE" << std::endl;
            PrintUsage();
            return 1;
        }

        // Key
        std::string key = argv[3];
        if( key.size() > 1024 )
        {
            std::cerr << "Error: <key> is too long, max size is 1024" << std::endl;
            PrintUsage();
            return 1;
        }


        // Value
        std::string value = argc < 5 ? "" : argv[4];
        if( key.size() > 1048576 ) // it's impossible, right?
        {
            std::cerr << "Error: <value> is too long, max size is 1048576" << std::endl;
            PrintUsage();
            return 1;
        }

        // Prepare data
        network::DecodedHeader dc{ command, static_cast< unsigned short >( key.size() ), static_cast< unsigned int >( value.size() ) };
        network::RequestHeader header{ dc };
        network::RequestFooter footer{};
        std::vector< char > data;
        data.resize( sizeof( network::RequestHeader ) + key.size() + value.size() + sizeof( network::RequestFooter ) );
        ::memcpy( &data[ 0 ],
                &header,
                sizeof( header ) );
        ::memcpy( &data[ sizeof( network::RequestHeader ) ],
                key.data(),
                key.size() );
        ::memcpy( &data[ sizeof( network::RequestHeader ) + key.size() ],
                value.data(),
                value.size() );
        ::memcpy( &data[ sizeof( network::RequestHeader ) + key.size() + value.size() ],
                &footer,
                sizeof( footer ) );

        std::cout << "data prepared: " << data.size() << " bytes" << std::endl;

        boost::asio::io_service io_service;

        boost::asio::ip::tcp::resolver resolver( io_service );
        boost::asio::ip::tcp::resolver::query query( boost::asio::ip::tcp::v4(), host, port );
        boost::asio::ip::tcp::resolver::iterator iterator = resolver.resolve( query );

        boost::asio::ip::tcp::socket s( io_service );
        boost::asio::connect( s, iterator );

        boost::asio::write( s, boost::asio::buffer( data ) );
        std::cout << "data written: " << data.size() << " bytes" << std::endl;

        char reply[ 1024 ];
        std::cout << "reading reply..." << std::endl;
        size_t reply_length = boost::asio::read( s, boost::asio::buffer( reply, 1024 ), boost::asio::transfer_at_least( 1 ) );
        std::cout << "received reply of size " << reply_length << " bytes" << std::endl;
        std::cout << "Reply: ";
        std::cout.write( reply, reply_length );
        std::cout << std::endl;
    }
    catch( std::exception const& e )
    {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}
