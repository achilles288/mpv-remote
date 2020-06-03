cmake_minimum_required(VERSION 3.10)

project("RMGraphics")

set(RMGAPHICS_VERSION 0.1.0)


set(CMAKE_CXX_STANDARD 17)


set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/$<0:>)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/$<0:>)

include_directories(${CMAKE_BINARY_DIR}/lib)


# Uses an external library libmpv
if(UNIX)
find_package(PkgConfig REQUIRED)
pkg_check_modules(MPV REQUIRED mpv)

else()
endif()


include_directories(${MPV_INCLUDE_DIRS})

add_executable(mpv-play mpv-play.c)
add_executable(mpv-remote mpv-remote.c)

target_link_libraries(mpv-play PUBLIC
    ${MPV_LIBRARIES}
)
