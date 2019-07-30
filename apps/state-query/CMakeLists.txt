#
# S T A T E   Q U E R Y
#
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(state-query)

setup_compiler()

add_executable(state-query main.cpp state_query.cpp state_query.hpp)
target_link_libraries(state-query PRIVATE fetch-ledger)
