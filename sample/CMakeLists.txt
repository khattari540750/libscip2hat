# ------------------------------------------------------------
#  libscip2hat CMake file for scip2hat
#  
#    auotmatically build
#  ~/sample
# ------------------------------------------------------------


# set include and link directory
include_directories(${PROJECT_SOURCE_DIR}/include)
link_directories(${PROJECT_SOURCE_DIR}/src)


# set ALL COMPLE OPTIONS 
set(CMAKE_C_FLAGS "-Wall -g")
set(CMAKE_CXX_FLAGS "-Wall -g")


# generated test-ms
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/sample)
add_executable(test_ms test_ms.c)
target_link_libraries (test_ms ${CMAKE_THREAD_LIBS_INIT} scip2hat)


# generated test-ms-callback
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/sample)
add_executable(test_ms_callback test_ms_callback.c)
target_link_libraries (test_ms_callback ${CMAKE_THREAD_LIBS_INIT} scip2hat)


# generated test-gs
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/sample)
add_executable(test_gs test_gs.c)
target_link_libraries (test_gs ${CMAKE_THREAD_LIBS_INIT} scip2hat)