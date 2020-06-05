cmake_minimum_required(VERSION 3.10)

project("MPV Remote")

set(MPV_REMOTE_VERSION 0.1.0)


set(CMAKE_CXX_STANDARD 17)


set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/$<0:>)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/$<0:>)

include_directories(${CMAKE_BINARY_DIR}/lib)




message(STATUS ${CMAKE_GENERATOR_PLATFORM})
# Uses an external library libmpv
if(UNIX)
find_package(PkgConfig REQUIRED)
pkg_check_modules(MPV REQUIRED mpv)


else()
file(COPY
    ${PROJECT_SOURCE_DIR}/mpv/bin/${CMAKE_GENERATOR_PLATFORM}/mpv-1.dll
    ${PROJECT_SOURCE_DIR}/mpv/bin/${CMAKE_GENERATOR_PLATFORM}/d3dcompiler_43.dll
    DESTINATION ${CMAKE_BINARY_DIR}
)
link_directories(${PROJECT_SOURCE_DIR}/mpv/lib/${CMAKE_GENERATOR_PLATFORM})
set(MPV_INCLUDE_DIRS mpv/include)
set(MPV_LIBRARIES mpv)
endif()




set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<0:>)

include_directories(${MPV_INCLUDE_DIRS})

add_executable(mpv-play mpv-play.c)
add_executable(mpv-remote mpv-remote.c)

target_link_libraries(mpv-play PUBLIC
    ${MPV_LIBRARIES}
)
