cmake_minimum_required(VERSION 3.10)

project("MPV Remote")

set(MPV_REMOTE_VERSION 1.1.0)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 11)

add_definitions(-D_CRT_SECURE_NO_WARNINGS)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/$<0:>)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/$<0:>)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/$<0:>)

link_directories(${CMAKE_BINARY_DIR}/lib)




# 
# Uses external libraries libmpv and json-c
# 
if(UNIX)
find_package(PkgConfig REQUIRED)
pkg_check_modules(MPV REQUIRED mpv)
pkg_check_modules(JSONC REQUIRED json-c)


else()
set(LIBMPV_DIR ${PROJECT_SOURCE_DIR}/external/libmpv)
file(COPY
    ${LIBMPV_DIR}/bin/${CMAKE_GENERATOR_PLATFORM}/mpv-1.dll
    ${LIBMPV_DIR}/bin/${CMAKE_GENERATOR_PLATFORM}/d3dcompiler_43.dll
    DESTINATION ${CMAKE_BINARY_DIR}/bin
)
link_directories(${LIBMPV_DIR}/lib/${CMAKE_GENERATOR_PLATFORM})
set(MPV_INCLUDE_DIRS ${LIBMPV_DIR}/include)
set(MPV_LIBRARIES mpv)

add_subdirectory(external/json-c)
set(JSONC_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/external/json-c)
set(JSONC_LIBRARIES json-c)
endif()


configure_file(
    ${PROJECT_SOURCE_DIR}/libremote/config.h.in
    ${PROJECT_SOURCE_DIR}/libremote/config.h
)

include_directories(
    ${CMAKE_BINARY_DIR}
    ${MPV_INCLUDE_DIRS}
    ${JSONC_INCLUDE_DIRS}
)




# 
# Common function library
# 
add_library(mpv-remote SHARED
    libremote/command.c
    libremote/command.h
    libremote/libremote.h
    libremote/logger.c
    libremote/logger.h
    libremote/status.c
    libremote/status.h
)

set_target_properties(mpv-remote PROPERTIES VERSION 1.1.0)
set_target_properties(mpv-remote PROPERTIES SOVERSION 1)

target_link_libraries(mpv-remote PUBLIC
    ${JSONC_LIBRARIES}
    ${MPV_LIBRARIES}
)


# 
# Remote program
# 
add_executable(remote
    remote/main.c
)

set_target_properties(remote
    PROPERTIES OUTPUT_NAME "mpv-remote"
)

target_link_libraries(remote PUBLIC mpv-remote)


# 
# Display program
# 
add_executable(player
    player/main.c
    player/player.c
    player/player.h
)

set_target_properties(player
    PROPERTIES OUTPUT_NAME "mpv-play"
)

target_link_libraries(player PUBLIC mpv-remote)




# 
# Unit tests
# 
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
set(gtest_force_shared_crt ON CACHE BOOL "compile gtest with MD" FORCE)
add_subdirectory(external/googletest)

set_target_properties(gtest
    PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/$<0:>
)
set_target_properties(gtest_main
    PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/$<0:>
)
set_target_properties(gtest
    PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/$<0:>
)
set_target_properties(gtest_main
    PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/$<0:>
)

set(MPV_REMOTE_TEST_RESOURCE_PATH ${PROJECT_SOURCE_DIR}/test)

configure_file(
    ${PROJECT_SOURCE_DIR}/test/config.h.in
    ${PROJECT_SOURCE_DIR}/test/test-config.h
)

add_executable(remote-test
    player/player.c
    test/command.cpp
    test/logger.cpp
    test/player.cpp
    test/status.cpp
)

target_include_directories(remote-test
    PRIVATE external/googletest/googletest/include
)

target_link_libraries(remote-test PUBLIC
    gtest
    gtest_main
    mpv-remote
)
