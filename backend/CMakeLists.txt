cmake_minimum_required(VERSION 3.14)
project(gcd_solver_service VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler optimizations for release
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# Windows specific settings
if(WIN32)
    add_definitions(-D_WIN32_WINNT=0x0601)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
endif()

# Main executable
add_executable(gcd_server
    main.cpp
)

# Link libraries
if(WIN32)
    target_link_libraries(gcd_server ws2_32)
else()
    find_package(Threads REQUIRED)
    target_link_libraries(gcd_server Threads::Threads)
endif()

# Install target
install(TARGETS gcd_server DESTINATION bin)
