cmake_minimum_required(VERSION 3.6)
project(Nayuki)

set(CMAKE_CXX_STANDARD 14)

set(OPENSSL_USE_STATIC_LIBS TRUE)
find_package(OpenSSL 1.1 REQUIRED)

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -pedantic)
endif()

add_library(nayuki
    common/FrameInfo.cpp
    common/FrameInfo.h
    common/SeekTable.cpp
    common/SeekTable.h
    common/StreamInfo.cpp
    common/StreamInfo.h
    common/Utilities.h
    decode/AbstractFlacLowLevelInput.cpp
    decode/AbstractFlacLowLevelInput.h
    decode/ByteArrayFlacInput.cpp
    decode/ByteArrayFlacInput.h
    decode/DataFormatException.h
    decode/FlacLowLevelInput.h
    encode/BitOutputStream.cpp
    encode/BitOutputStream.h
)
target_link_libraries(nayuki OpenSSL::Crypto)