cmake_minimum_required(VERSION 3.14)

project(kvdb_data_models LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(kvdb_data_models STATIC
  kvdb_data_models.cpp
  kvdb_data_models.hpp
)

target_compile_definitions(kvdb_data_models PRIVATE KVDB_DATA_MODELS_LIBRARY)
