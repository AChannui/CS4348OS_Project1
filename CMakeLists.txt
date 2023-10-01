cmake_minimum_required(VERSION 3.24)
project(CS4348OS)

set(CMAKE_CXX_STANDARD 17)

add_executable(simulator main.cpp )

add_executable(memory  memory.cpp)

configure_file(samples/sample1.txt sample1.txt COPYONLY)
configure_file(samples/sample2.txt sample2.txt COPYONLY)
configure_file(samples/sample3.txt sample3.txt COPYONLY)
configure_file(samples/sample4.txt sample4.txt COPYONLY)
