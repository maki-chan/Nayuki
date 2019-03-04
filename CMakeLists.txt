cmake_minimum_required(VERSION 3.6)
project(Nayuki)

set(CMAKE_CXX_STANDARD 14)

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -pedantic)
endif()

add_library(nayuki
        common/FrameInfo.cpp
        common/FrameInfo.h
        decode/DataFormatException.h
        decode/FlacLowLevelInput.h
        encode/BitOutputStream.cpp
        encode/BitOutputStream.h
        common/SeekTable.h
        common/SeekTable.cpp common/Utilities.h)
