cmake_minimum_required(VERSION 3.0 FATAL_ERROR)
project(usefulmacros)
set(CMAKE_cpp_STANDARD 11)
set(CMAKE_BUILD_TYPE debug)

list(APPEND CMAKE_PREFIX_PATH $ENV{ROOTSYS})

find_package(ROOT REQUIRED COMPONENTS RIO)
find_package(Boost COMPONENTS program_options REQUIRED)
include(${ROOT_USE_FILE})
include_directories(${ROOT_INCLUDE_DIRS} ${Boost_INCLUDE_DIR})

add_executable(compareRootFiles compareRootFiles.C)
target_link_libraries(compareRootFiles ${Boost_LIBRARIES} ${ROOT_LIBRARIES})