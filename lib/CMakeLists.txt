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
                      opencl)

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

# blanket set the include directories for all subdirectories added here
# so we do not need to do it in each subdirectory's CMakeLists
include_directories(${JHMR_LIB_INC_DIRS})

# build the sub-components
foreach (JHMR_CUR_LIB_DIR ${JHMR_LIB_SUB_DIRS})
  add_subdirectory(${JHMR_CUR_LIB_DIR})
endforeach()

# create a list of all of the object files created
set(JHMR_LIB_OBJS)
foreach (JHMR_CUR_LIB ${JHMR_LIB_SUB_DIRS})
  list(APPEND JHMR_LIB_OBJS $<TARGET_OBJECTS:jhmr_${JHMR_CUR_LIB}>)
endforeach()

# combine the subcomponents into a single library
add_library(jhmr STATIC ${JHMR_LIB_OBJS})

# These properties are propogated to the executables in the apps directory
target_include_directories(jhmr PUBLIC ${JHMR_LIB_INC_DIRS})
target_link_libraries(jhmr PUBLIC ${JHMR_EXT_LIBS_TO_LINK})

