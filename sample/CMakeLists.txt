# ------------------------------------------------------------
#  libscip2hat CMake file for scip2hat
#  
#    auotmatically build
#  ~/sample
# ------------------------------------------------------------


# set include directory
include_directories(${PROJECT_SOURCE_DIR}/include)


# set ALL COMPLE OPTIONS 
set(CMAKE_C_FLAGS "-Wall -g")
set(CMAKE_CXX_FLAGS "-Wall -g")


# generated test-ms
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/sample)
add_executable(test-ms test-ms.c)
target_link_libraries (test-ms pthread scip2hat)


# generated test-ms-callback
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/sample)
add_executable(test-ms-callback test-ms-callback.c)
target_link_libraries (test-ms-callback pthread scip2hat)


# generated test-gs
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/sample)
add_executable(test-gs test-gs.c)
target_link_libraries (test-gs pthread scip2hat)