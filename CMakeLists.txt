project(final)

set(CMAKE_CXX_STANDARD 11)

add_executable(final main.cpp)

find_package(Threads REQUIRED)
if(THREADS_HAVE_PTHREAD_ARG)
    set_property(TARGET final PROPERTY COMPILE_OPTIONS "-pthread")
    set_property(TARGET final PROPERTY INTERFACE_COMPILE_OPTIONS "-pthread")
endif()
if(CMAKE_THREAD_LIBS_INIT)
    target_link_libraries(final "${CMAKE_THREAD_LIBS_INIT}")
endif()