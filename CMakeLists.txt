CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
project(monitor)
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -g --std=c++17")

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
  SRC_DEBUG
  src/debug/*.cc
  )

file(  
  GLOB_RECURSE
  SRC_SERVER
  src/server/*.cc
)

find_package(nlohmann_json 3.7.3 REQUIRED)
add_executable (dio-monitor ${SRC_COMMON} ${SRC_SERVER})
add_executable (dio-client ${SRC_COMMON} ${SRC_CLIENT})
add_executable (dio-debug ${SRC_COMMON} ${SRC_DEBUG})

target_link_libraries (dio-monitor -lpthread -lbfd -lvirt nlohmann_json::nlohmann_json)
target_link_libraries (dio-client -lpthread -lbfd -lvirt  nlohmann_json::nlohmann_json)
target_link_libraries (dio-debug -lpthread -lbfd -lvirt  nlohmann_json::nlohmann_json)

include_directories(${CMAKE_SOURCE_DIR}/src/)
install (TARGETS dio-client dio-monitor DESTINATION /usr/bin/)








