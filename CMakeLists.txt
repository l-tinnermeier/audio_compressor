cmake_minimum_required(VERSION 3.16)

project(AudioCompressor LANGUAGES CXX)

add_executable(audio_compressor
               src/main.cpp
               src/wav.cpp
               src/compression.cpp
               src/decompression.cpp)

target_include_directories(audio_compressor PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_compile_features(audio_compressor PRIVATE cxx_std_17)

set_target_properties(audio_compressor PROPERTIES CXX_EXTENSIONS OFF)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  target_compile_options(audio_compressor PRIVATE -Wall -Wextra -Wpedantic)
endif()
