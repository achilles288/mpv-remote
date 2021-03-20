cmake_minimum_required(VERSION 3.10)

project("MPV Remote" VERSION 1.1.0)

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
pkg_check_modules(MHD REQUIRED libmicrohttpd)
pkg_check_modules(GCRY REQUIRED libgcrypt)
pkg_check_modules(JSONC REQUIRED json-c)


else()
set(JSONC_INCLUDE_DIRS external/include/json-c)
set(JSONC_LIBRARIES json-c)
set(MPV_INCLUDE_DIRS external/include/libmpv)
set(MPV_LIBRARIES mpv)
set(MHD_INCLUDE_DIRS external/include/libmicrohttpd)
set(MHD_LIBRARIES libmicrohttpd-dll)

link_directories(external/lib)

file(COPY
    external/bin
    DESTINATION ${CMAKE_BINARY_DIR}
)
endif()


configure_file(
    ${PROJECT_SOURCE_DIR}/libremote/config.h.in
    ${PROJECT_SOURCE_DIR}/libremote/config.h
)

include_directories(
    ${CMAKE_BINARY_DIR}
    ${MPV_INCLUDE_DIRS}
    ${MHD_INCLUDE_DIRS}
    ${GCRY_INCLUDE_DIRS}
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

set_target_properties(mpv-remote PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

target_link_libraries(mpv-remote PUBLIC ${JSONC_LIBRARIES})


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
    player/http/auth.c
    player/http/auth.h
    player/http/con_type.h
    player/http/get.c
    player/http/http.c
    player/http/http.h
    player/http/post.c
    player/player.c
    player/player.h
)

set_target_properties(player
    PROPERTIES OUTPUT_NAME "mpv-play"
)

target_link_libraries(player PUBLIC
    mpv-remote
    ${MPV_LIBRARIES}
    ${MHD_LIBRARIES}
    ${GCRY_LIBRARIES}
)




# 
# Copy the data files
# 
file(COPY
    http
    DESTINATION ${CMAKE_BINARY_DIR}/bin
)
