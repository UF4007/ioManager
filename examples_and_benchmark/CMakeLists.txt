cmake_minimum_required(VERSION 3.14)
project(ioManagerExamples CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

include_directories(${CMAKE_SOURCE_DIR}/../../include)

file(GLOB_RECURSE EXAMPLES
    "core/*.cpp"
    "socket/*.cpp"
    "protocol/chan/*.cpp"
    "protocol/http/*.cpp"
    "protocol/kcp/*.cpp"
)

foreach(example ${EXAMPLES})
    get_filename_component(example_name ${example} NAME_WE)
    add_executable(${example_name} ${example})
    if(UNIX)
        target_link_libraries(${example_name} pthread)
    endif()
endforeach()
