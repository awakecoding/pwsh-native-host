cmake_minimum_required(VERSION 3.2)

project(native-host C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

if(WIN32)
	set(C_FLAGS "")
	set(C_FLAGS "${C_FLAGS} -D_UNICODE")
	set(C_FLAGS "${C_FLAGS} -D_CRT_SECURE_NO_WARNINGS")
	set(C_FLAGS "${C_FLAGS} -DWIN32_LEAN_AND_MEAN")
	set(C_FLAGS "${C_FLAGS} -D_WINSOCK_DEPRECATED_NO_WARNINGS")
	set(C_FLAGS "${C_FLAGS} -DWINVER=0x0601 -D_WIN32_WINNT=0x0601")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${C_FLAGS}")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${C_FLAGS}")
endif()

if(CMAKE_COMPILER_IS_GNUCC)
	set(C_FLAGS "${C_FLAGS} -Wformat-truncation=0")
endif()

include_directories(include)

add_executable(native-host
	main.c)

#set(NETHOST_LIBRARY "${CMAKE_SOURCE_DIR}/nethost/libnethost.a")

target_link_libraries(native-host
	${CMAKE_DL_LIBS} Threads::Threads)
