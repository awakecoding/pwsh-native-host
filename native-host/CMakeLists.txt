cmake_minimum_required(VERSION 3.2)

project(native-host C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

include_directories(include)

add_executable(native-host
	main.c)

#set(NETHOST_LIBRARY "${CMAKE_SOURCE_DIR}/nethost/libnethost.a")

target_link_libraries(native-host
	${CMAKE_DL_LIBS} Threads::Threads)
