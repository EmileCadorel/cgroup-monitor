CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
project(monitor)
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -O3 --std=c++17")

file(  
  GLOB_RECURSE
  SRC
  src/*.cc
)

add_executable (monitor ${SRC})
target_link_libraries (monitor -lpthread -lbfd)
include_directories(${CMAKE_SOURCE_DIR}/src/)









