cmake_minimum_required(VERSION 3.27)

project(amba-tlm CXX)

set(CUSTOM_CXX_FLAGS -static-libstdc++ -Wall -Werror)

########## Build Library: libarmtlmaxi4
file(GLOB_RECURSE armtlmaxi4_sources src/libarmaxi4.cpp)
file(GLOB_RECURSE armtlmaxi4_headers include/*arm_axi4*.h include/*arm_tlm*.h)

add_library(armtlmaxi4)
target_compile_options(armtlmaxi4 PRIVATE ${CUSTOM_CXX_FLAGS})
target_include_directories(armtlmaxi4 PUBLIC include)
target_sources(armtlmaxi4 PUBLIC ${armtlmaxi4_sources})

########## Install Package Library: libarmtlmaxi4
target_sources(armtlmaxi4 PUBLIC FILE_SET HEADERS BASE_DIRS include FILES ${armtlmaxi4_headers})
install(TARGETS armtlmaxi4 FILE_SET HEADERS)


########## Build Library: libarmtlmchi
file(GLOB_RECURSE armtlmchi_sources src/libarmchi.cpp)
file(GLOB_RECURSE armtlmchi_headers include/*arm_chi*.h include/*arm_tlm*.h)

add_library(armtlmchi)
target_compile_options(armtlmchi PRIVATE ${CUSTOM_CXX_FLAGS})
target_include_directories(armtlmchi PUBLIC include)
target_sources(armtlmchi PUBLIC ${armtlmchi_sources})

########## Install Package Library: libarmtlmchi
target_sources(armtlmchi PUBLIC FILE_SET HEADERS BASE_DIRS include FILES ${armtlmchi_headers})
install(TARGETS armtlmchi FILE_SET HEADERS)
