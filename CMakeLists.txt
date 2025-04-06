cmake_minimum_required(VERSION 3.13)

project(codecrafters-git)

file(GLOB_RECURSE SOURCE_FILES src/*.c src/*.h)

set(CMAKE_C_STANDARD 23) # Enable the C23 standard

add_executable(git ${SOURCE_FILES}
        src/cat_file.c
        src/cat_file.h
        src/debug_helpers.h
        src/hash_object.c
        src/hash_object.h
        src/git_dir_helpers.c
        src/git_dir_helpers.h
        src/ls_tree.c
        src/ls_tree.h
        src/compression.c
        src/compression.h)

set(ZLIBPATH "/usr/local")
target_include_directories(git PRIVATE ${ZLIBPATH}/include)
target_link_directories(git PRIVATE ${ZLIBPATH}/lib)
target_link_libraries(git PRIVATE libz.so)

target_link_libraries(git PRIVATE ssl)
target_link_libraries(git PRIVATE crypto)
