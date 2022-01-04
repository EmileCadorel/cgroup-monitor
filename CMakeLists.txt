CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
project(monitor)
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -O3 --std=c++17")

file(  
  GLOB_RECURSE
  SRC_COMMON
  src/monitor/*.cc
  )

file(  
  GLOB_RECURSE
  SRC_CLIENT
  src/client/*.cc
  )

file(  
  GLOB_RECURSE
  SRC_SERVER
  src/server/*.cc
)

add_executable (monitor ${SRC_COMMON} ${SRC_SERVER})
add_executable (client ${SRC_COMMON} ${SRC_CLIENT})
target_link_libraries (monitor -lpthread -lbfd)
target_link_libraries (client -lpthread -lbfd)

include_directories(${CMAKE_SOURCE_DIR}/src/)









