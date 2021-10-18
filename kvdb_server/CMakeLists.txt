cmake_minimum_required(VERSION 3.5)

project(kvdb_server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(BOOST_ROOT "C:\\mingw-w64\\boost_1_77_0")
find_package(Boost 1.77 REQUIRED COMPONENTS
             program_options filesystem date_time)

add_executable(kvdb_server
    kvdb_server_main.cpp
    kvdb_server_network.cpp
    kvdb_server_network.hpp
    kvdb_server_storage.cpp
    kvdb_server_storage.hpp
    kvdb_server_st_m.cpp
    kvdb_server_st_m.hpp
    kvdb_server_st_p.cpp
    kvdb_server_st_p.hpp
    kvdb_server_stats.cpp
    kvdb_server_stats.hpp
    )

target_link_libraries(kvdb_server Boost::program_options Boost::filesystem Boost::date_time wsock32 ws2_32 kvdb_data_models)
