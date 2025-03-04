cmake_minimum_required(VERSION 3.13)

project(codecrafters-git)

file(GLOB_RECURSE SOURCE_FILES src/*.c src/*.h)

set(CMAKE_C_STANDARD 23) # Enable the C23 standard

add_executable(git ${SOURCE_FILES}
        src/cat-file.c
        src/cat-file.h)

set(ZLIBPATH "/usr/local")
target_include_directories(git PRIVATE ${ZLIBPATH}/include)
target_link_directories(git PRIVATE ${ZLIBPATH}/lib)
target_link_libraries(git PRIVATE libz.so)
