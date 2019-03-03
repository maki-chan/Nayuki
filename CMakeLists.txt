cmake_minimum_required(VERSION 3.9)
project(Nayuki)

set(CMAKE_CXX_STANDARD 14)

add_library(nayuki
    common/FrameInfo.cpp
    common/FrameInfo.h
    decode/DataFormatException.h
    decode/FlacLowLevelInput.h
    encode/BitOutputStream.cpp
    encode/BitOutputStream.h
)
