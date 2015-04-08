# ------------------------------------------------------------
#  libssm and ssm-coodinator CMake file for scip2hat
#  
#    auotmatically build
#  ~/src directory
# ------------------------------------------------------------


# set include directory
include_directories(${PROJECT_SOURCE_DIR}/include)


# set ALL COMPLE OPTIONS 
set(CMAKE_C_FLAGS "-Wall")
set(CMAKE_CXX_FLAGS "-Wall")


# generated libssm
set(SSM_LIBS ${SSM_LIBS} ssm)
#set(CMAKE_C_FLAGS "-Werror")
#set(CMAKE_CXX_FLAGS "-Werror")
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/src)
add_library(ssm libssm-shm.c libssm-time.c libssm.c)
target_link_libraries (ssm pthread)


# generated ssm-coodinator
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/src)
add_executable(ssm-coordinator ssm-coordinator.cpp)
target_link_libraries(ssm-coordinator ${SSM_LIBS})


# install lib and executable
#install(TARGETS MathFunctions DESTINATION bin)
#install(FILES MathFunctions.h DESTINATION include)
