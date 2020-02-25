cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(lane-server)

setup_compiler()

add_executable(lane-server main.cpp)

target_link_libraries(lane-server
                      PRIVATE fetch-ledger
                              fetch-constellation
                              fetch-http
                              fetch-beacon
                              fetch-settings
                              fetch-version)
