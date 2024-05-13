cmake_minimum_required(VERSION 3.20)

project(amba_tlm_demo CXX)

find_package(SystemCLanguage CONFIG REQUIRED)
find_package(amba-tlm CONFIG REQUIRED)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CUSTOM_CXX_FLAGS -Wall -Werror -Wno-format -Wno-unused-variable)

# Main Executable
file(GLOB TRAFFIC_EXAMPLE_SOURCES
    src/Monitor.cpp
    src/Memory.cpp
    src/TrafficGenerator.cpp
    src/TrafficExample.cpp)

file(GLOB TRANSACTOR_EXAMPLE_SOURCES
    src/Monitor.cpp
    src/Memory.cpp
    src/TrafficGenerator.cpp
    src/Transactors.cpp
    src/TransactorExample.cpp)

get_target_property(AMBA_TLM_INCLUDE_DIRS amba-tlm::amba-tlm INTERFACE_INCLUDE_DIRECTORIES)
get_target_property(SYSTEMC_INCLUDE_DIRS SystemC::systemc INTERFACE_INCLUDE_DIRECTORIES)

add_executable(TrafficExample ${TRAFFIC_EXAMPLE_SOURCES})
target_include_directories(TrafficExample PUBLIC include ${SYSTEMC_INCLUDE_DIRS} ${AMBA_TLM_INCLUDE_DIRS})
target_compile_options(TrafficExample PRIVATE ${CUSTOM_CXX_FLAGS})
target_link_libraries(TrafficExample SystemC::systemc amba-tlm::amba-tlm)

add_executable(TransactorExample ${TRANSACTOR_EXAMPLE_SOURCES})
target_include_directories(TransactorExample PUBLIC include ${SYSTEMC_INCLUDE_DIRS} ${AMBA_TLM_INCLUDE_DIRS})
target_compile_options(TransactorExample PRIVATE ${CUSTOM_CXX_FLAGS})
target_link_libraries(TransactorExample SystemC::systemc amba-tlm::amba-tlm)
