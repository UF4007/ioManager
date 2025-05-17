cmake_minimum_required(VERSION 3.10)

project(ioManager)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/ioManager/
        DESTINATION ${CMAKE_INSTALL_PREFIX}/include/ioManager)

add_library(ioManager INTERFACE)