# ------------------------------------------------------------
#  libscip2hat CMake file for scip2hat
#  
#    auotmatically build and install
#  ~/src directory
# ------------------------------------------------------------


# set include directory
include_directories(${PROJECT_SOURCE_DIR}/include)


# set ALL COMPLE OPTIONS 
set(CMAKE_C_FLAGS "-Wall -O3 -Werror")
set(CMAKE_CXX_FLAGS "-Wall -O3 -Werror")


# generated libscip2hat
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/src)
add_library(scip2hat libscip2hat_base.c libscip2hat_cmd.c libscip2hat_dbuffer.c)
target_link_libraries (scip2hat)


# install library
#install(TARGETS scip2hat DESTINATION lib)
