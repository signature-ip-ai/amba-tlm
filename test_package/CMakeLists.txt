cmake_minimum_required(VERSION 3.20)

project(amba_tlm_demo CXX)

find_package(SystemCLanguage CONFIG REQUIRED)
find_package(amba-tlm CONFIG REQUIRED)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CUSTOM_CXX_FLAGS -Wall -Werror -Wno-format -Wno-unused-variable)

########## Main Executables
get_target_property(AMBA_TLM_AXI4_INCLUDE_DIRS amba-tlm::armtlmaxi4 INTERFACE_INCLUDE_DIRECTORIES)
get_target_property(SYSTEMC_INCLUDE_DIRS SystemC::systemc INTERFACE_INCLUDE_DIRECTORIES)

file(GLOB AXI_TRAFFIC_EXAMPLE_SOURCES
    src/axi/AXIMonitor.cpp
    src/axi/AXIMemory.cpp
    src/axi/AXITrafficGenerator.cpp
    src/axi/AXITrafficExample.cpp)

add_executable(AXITrafficExample ${AXI_TRAFFIC_EXAMPLE_SOURCES})
target_include_directories(AXITrafficExample PUBLIC include/axi ${SYSTEMC_INCLUDE_DIRS} ${AMBA_TLM_AXI4_INCLUDE_DIRS})
target_compile_options(AXITrafficExample PRIVATE ${CUSTOM_CXX_FLAGS})
target_link_libraries(AXITrafficExample SystemC::systemc amba-tlm::armtlmaxi4)


file(GLOB AXI_TRANSACTOR_EXAMPLE_SOURCES
    src/axi/AXIMonitor.cpp
    src/axi/AXIMemory.cpp
    src/axi/AXITrafficGenerator.cpp
    src/axi/AXITransactors.cpp
    src/axi/AXITransactorExample.cpp)

add_executable(AXITransactorExample ${AXI_TRANSACTOR_EXAMPLE_SOURCES})
target_include_directories(AXITransactorExample PUBLIC include/axi ${SYSTEMC_INCLUDE_DIRS} ${AMBA_TLM_AXI4_INCLUDE_DIRS})
target_compile_options(AXITransactorExample PRIVATE ${CUSTOM_CXX_FLAGS})
target_link_libraries(AXITransactorExample SystemC::systemc amba-tlm::armtlmaxi4)

get_target_property(AMBA_TLM_CHI_INCLUDE_DIRS amba-tlm::armtlmchi INTERFACE_INCLUDE_DIRECTORIES)

file(GLOB CHI_TRAFFIC_EXAMPLE_SOURCES
    src/chi/CHIMonitor.cpp
    src/chi/CHIMemory.cpp
    src/chi/CHITrafficGenerator.cpp
    src/chi/CHITrafficExample.cpp)

add_executable(CHITrafficExample ${CHI_TRAFFIC_EXAMPLE_SOURCES})
target_include_directories(CHITrafficExample PUBLIC include/chi ${SYSTEMC_INCLUDE_DIRS} ${AMBA_TLM_CHI_INCLUDE_DIRS})
target_compile_options(CHITrafficExample PRIVATE ${CUSTOM_CXX_FLAGS})
target_link_libraries(CHITrafficExample SystemC::systemc amba-tlm::armtlmchi)
