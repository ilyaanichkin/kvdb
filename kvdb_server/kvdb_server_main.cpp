#include <iostream>
#include <boost/program_options.hpp>
#include "kvdb_server_storage.hpp"
#include "kvdb_server_stats.hpp"
#include <boost/interprocess/exceptions.hpp>

namespace network
{

void RunAsioServer( boost::asio::io_service& io_service, size_t port, size_t threads, storage::IStorage& strg, stats::IStats& stats );

} // namespace network

int main( int argc, char *argv[] )
{
    size_t v_port = 0, v_threads = 0, v_size = 0;
    boost::program_options::options_description od( "Allowed options" );
    od.add_options()
            ( "help,h", "Show this help message" )
            ( "port,p", boost::program_options::value< size_t >( &v_port )->default_value( 0 ), "TCP listener port number" )
            ( "threads,t", boost::program_options::value< size_t >( &v_threads )->default_value( 1 ), "Number of threads for request processing" )
            ( "size,s", boost::program_options::value< size_t >( &v_size )->default_value( 1 ), "Storage size in megabytes" )
            ;
    boost::program_options::variables_map vm;
    try {
        boost::program_options::store(
                    boost::program_options::command_line_parser( argc, argv )
                    .options( od )
                    .positional( {} )
                    .allow_unregistered()
                    .run(),
                    vm );
        boost::program_options::notify( vm );
    }
    catch( boost::exception const& ex )
    {
        std::cerr << "boost::exception: " << std::endl;
    }
    catch( std::exception const& ex )
    {
        std::cerr << "std::exception: " << ex.what() << std::endl;
    }
    catch( ... )
    {
        std::cerr << "..." << std::endl;
    }

    if( vm.count( "help" ) > 0 )
    {
        std::cout << od << std::endl;
        return 0;
    }
    size_t port = vm[ "port" ].as< size_t >();
    size_t size = vm[ "size" ].as< size_t >();
    size_t threads = vm[ "threads" ].as< size_t >();
    if( port < 1024 || port > 49151 )
    {
        std::srand( static_cast< unsigned int >( std::time( 0 ) ) );
        port = ( static_cast< size_t >( std::rand() ) * ( 49151 - 1024 ) ) / RAND_MAX + 1024;
        std::cout << "Warning: TCP port was assigned to a random value " << port << " from range [1024..49151]" << std::endl;
    }
    if( threads < 1 || threads > 10 )
    {
        if( threads < 1 )
            threads = 1;
        if( threads > 10 )
            threads = 10;
        std::cout << "Warning: number of processing threads is set to " << threads << ", allowed range is [1..10]" << std::endl;
    }
    if( size < 1 )
    {
        if( size < 1 )
            size = 1;
        if( size > 1024 )
            size = 1024;
        std::cout << "Warning: storage size is set to " << size << " MB, allowed range is [1..1024]" << std::endl;
    }

    std::unique_ptr< storage::IStorage > strg = nullptr;
    try
    {
        strg = storage::InitializeStorage( size * 1024 * 1024, storage::IStorage::tPersistent );
    }
    catch( boost::interprocess::interprocess_exception const& ex )
    {
        std::cout << "interprocess_exception: " << ex.what()
                  << " code " << ex.get_error_code()
                  << " nc " << ex.get_native_error()
                  << std::endl;
        strg = nullptr;
    }
    if( strg == nullptr )
    {
        std::cout << "Error: could not create storage" << std::endl;
        return 1;
    }

    boost::asio::io_service io_service;

    stats::Stats stats_reporter( io_service, *strg, 60 ); // 60 seconds
    stats_reporter.Launch();

    network::RunAsioServer( io_service, port, threads, *strg, stats_reporter );

    return 0;
}
