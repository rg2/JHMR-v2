
add_library(cmaes STATIC boundary_transformation.c cmaes.c)
target_include_directories(cmaes PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

