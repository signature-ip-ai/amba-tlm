cmake_minimum_required(VERSION 3.27)

project(armtlmaxi4 CXX)

file(GLOB_RECURSE sources src/*.cpp)
file(GLOB_RECURSE headers include/*.h)

set(CUSTOM_CXX_FLAGS -static-libstdc++ -Wall -Werror -Wno-format -Wno-class-memaccess -Wno-error=unused-variable)

########## Library
add_library(${PROJECT_NAME})
target_compile_options(${PROJECT_NAME} PRIVATE ${CUSTOM_CXX_FLAGS})
target_include_directories(${PROJECT_NAME} PUBLIC include)
target_sources(${PROJECT_NAME} PUBLIC ${sources})

########## Install Package Library
target_sources(${PROJECT_NAME} PUBLIC FILE_SET HEADERS BASE_DIRS include FILES ${headers})
install(TARGETS ${PROJECT_NAME} FILE_SET HEADERS)
