#
# S T O R A G E   U N I T   C L I
#
cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(storage-unit-cli)

setup_compiler()

add_executable(tx-ctl
               tx_ctl.cpp
               tx_storage_client.cpp
               tx_storage_client.hpp
               tx_storage_tool.cpp
               tx_storage_tool.hpp)
target_link_libraries(tx-ctl PRIVATE fetch-ledger)

add_executable(tx-gen tx_gen.cpp)
add_executable(tx-gen_nathans-bestcase-scenario-even-lane-distribution tx-gen_nathans-bestcase-scenario-even-lane_distribution.cpp)
add_executable(test_program test_program.cpp)

target_link_libraries(tx-gen PRIVATE fetch-ledger)
target_link_libraries(tx-gen_nathans-bestcase-scenario-even-lane-distribution PRIVATE fetch-ledger)
target_link_libraries(test_program PRIVATE fetch-ledger)
