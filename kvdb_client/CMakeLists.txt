cmake_minimum_required(VERSION 3.5)

project(kvdb_client LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(BOOST_ROOT "C:\\mingw-w64\\boost_1_77_0")
find_package(Boost 1.77 REQUIRED COMPONENTS
             program_options)

add_executable(kvdb_client kvdb_client_main.cpp)

target_link_libraries(kvdb_client Boost::program_options wsock32 ws2_32 kvdb_data_models)
