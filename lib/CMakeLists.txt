# MIT License
#
# Copyright (c) 2020 Robert Grupp
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

# set the library sub-components/directories here
set(JHMR_LIB_SUB_DIRS common
                      file_formats
                      itk
                      vtk
                      hdf5
                      opencv
                      image
                      transforms
                      regi
                      basic_math
                      spatial
                      optim
                      hip_surgery
                      ray_cast
                      opencl
                      version)

# build up the list of include directories for the jhmr library
set(JHMR_LIB_INC_DIRS)
foreach (JHMR_CUR_LIB_DIR ${JHMR_LIB_SUB_DIRS})
  list(APPEND JHMR_LIB_INC_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/${JHMR_CUR_LIB_DIR}")
endforeach()

# This is needed for any subdirectories of entries in JHMR_LIB_SUB_DIRS
# Do something more intelligent with a for loop when more sub-subdirectories are added
# OR restructure the library to only have one include directory at the root and
# switch to including longer paths
list(APPEND JHMR_LIB_INC_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/regi/sim_metrics_2d"
                              "${CMAKE_CURRENT_SOURCE_DIR}/regi/pnp_solvers"
                              "${CMAKE_CURRENT_SOURCE_DIR}/regi/penalty_fns_2d_3d"
                              "${CMAKE_CURRENT_SOURCE_DIR}/regi/interfaces_2d_3d")

# this will be the list of all of the object files created
set(JHMR_LIB_OBJS)

# build the sub-components
# an object library is created in each subdirectory, e.g. jhmr_basic_math, jhmr_optim
foreach (JHMR_CUR_LIB_DIR ${JHMR_LIB_SUB_DIRS})
  add_subdirectory(${JHMR_CUR_LIB_DIR})

  target_include_directories(jhmr_${JHMR_CUR_LIB_DIR} PRIVATE ${JHMR_LIB_INC_DIRS})
  
  target_link_libraries(jhmr_${JHMR_CUR_LIB_DIR}
                        PUBLIC ${JHMR_EXT_LIBS_TO_LINK}
                        PRIVATE cmaes)
  
  # add to the list of all of the object files created
  list(APPEND JHMR_LIB_OBJS $<TARGET_OBJECTS:jhmr_${JHMR_CUR_LIB_DIR}>)
endforeach()

# combine the subcomponents into a single library
add_library(jhmr ${JHMR_LIB_OBJS})

# IMPORTANT NOTE: Since we are providing a list of include directories,
# the $<BUILD_INTERFACE:${JHMR_LIB_INC_DIRS}> must be surrounded by quotes.
# Otherwise the include directories are not propagated to other targets
# dependent on the jhmr target (e.g. executables) in this build tree.
# e.g. "$<BUILD_INTERFACE:${JHMR_LIB_INC_DIRS}>"
target_include_directories(jhmr PUBLIC
                                  "$<BUILD_INTERFACE:${JHMR_LIB_INC_DIRS}>"
                                  $<INSTALL_INTERFACE:include>)

target_link_libraries(jhmr PUBLIC ${JHMR_EXT_LIBS_TO_LINK} PRIVATE cmaes)

file(GLOB_RECURSE JHMR_ALL_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
set_target_properties(jhmr PROPERTIES PUBLIC_HEADER "${JHMR_ALL_HEADERS}")

