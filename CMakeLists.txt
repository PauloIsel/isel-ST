cmake_minimum_required(VERSION 3.22.1)
project(ST)

project(ST CXX)
set(CMAKE_CXX_STANDARD 17)

set(PATH ${CMAKE_SOURCE_DIR}/ST/Example ST)

file(GLOB st "${PATH}/Example ST/*.cpp")


add_executable(exampleSemNet ${st})
