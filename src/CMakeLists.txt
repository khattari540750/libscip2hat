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


# generate and install libscip2hat ver. static 
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/src)
add_library(scip2hatStatic STATIC libscip2hat_base.c libscip2hat_cmd.c libscip2hat_dbuffer.c)
set_target_properties(scip2hatStatic PROPERTIES OUTPUT_NAME scip2hat)
install(TARGETS scip2hatStatic DESTINATION lib)


# generate and install libscip2hat ver. shared
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/src)
add_library(scip2hatShared SHARED libscip2hat_base.c libscip2hat_cmd.c libscip2hat_dbuffer.c)
set_target_properties(scip2hatShared PROPERTIES OUTPUT_NAME scip2hat)
install(TARGETS scip2hatShared DESTINATION lib)
