cmake_minimum_required(VERSION 3.10)

project(ioManager)

if(EXISTS "${CMAKE_INSTALL_PREFIX}/include/ioManager")
    file(REMOVE_RECURSE "${CMAKE_INSTALL_PREFIX}/include/ioManager")
    message("Remove the previous version of ioManager.")
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/ioManager/
        DESTINATION ${CMAKE_INSTALL_PREFIX}/include/ioManager)

add_library(ioManager INTERFACE)