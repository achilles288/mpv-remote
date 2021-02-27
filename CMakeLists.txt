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
set(JSONC_DIR ${PROJECT_SOURCE_DIR}/external/json-c)
file(COPY
    ${JSONC_DIR}/bin/${CMAKE_GENERATOR_PLATFORM}/json-c.dll
    DESTINATION ${CMAKE_BINARY_DIR}/bin
)
link_directories(${JSONC_DIR}/lib/${CMAKE_GENERATOR_PLATFORM})
set(JSONC_INCLUDE_DIRS ${JSONC_DIR}/include)
set(JSONC_LIBRARIES json-c)

set(LIBMPV_DIR ${PROJECT_SOURCE_DIR}/external/libmpv)
file(COPY
    ${LIBMPV_DIR}/bin/${CMAKE_GENERATOR_PLATFORM}/mpv-1.dll
    ${LIBMPV_DIR}/bin/${CMAKE_GENERATOR_PLATFORM}/d3dcompiler_43.dll
    DESTINATION ${CMAKE_BINARY_DIR}/bin
)
link_directories(${LIBMPV_DIR}/lib/${CMAKE_GENERATOR_PLATFORM})
set(MPV_INCLUDE_DIRS ${LIBMPV_DIR}/include)
set(MPV_LIBRARIES mpv)

set(LIBMHD_DIR ${PROJECT_SOURCE_DIR}/external/libmicrohttpd)
file(COPY
    ${LIBMHD_DIR}/bin/${CMAKE_GENERATOR_PLATFORM}/libmicrohttpd-dll.dll
    DESTINATION ${CMAKE_BINARY_DIR}/bin
)
link_directories(${LIBMHD_DIR}/lib/${CMAKE_GENERATOR_PLATFORM})
set(MHD_INCLUDE_DIRS ${LIBMHD_DIR}/include)
set(MHD_LIBRARIES libmicrohttpd-dll)
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
    player/http.c
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
