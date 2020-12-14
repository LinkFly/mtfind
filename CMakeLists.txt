cmake_minimum_required(VERSION 3.2)

project(mtfind VERSION 0.0.1)

find_package(Threads REQUIRED)

add_executable(mtfind mtfind.cpp)

set_target_properties(mtfind PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
	IF(WIN32)
		#COMPILE_OPTIONS "-Wall" # A lot of messages (in std libraries)
	ELSE()
		COMPILE_OPTIONS "-Wpedantic;-Wall;-Wextra"
	ENDIF()
)

target_link_libraries(mtfind
    Threads::Threads
)
