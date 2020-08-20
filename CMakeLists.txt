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

cmake_minimum_required(VERSION 3.0.0 FATAL_ERROR)

project(jhmr-v2)

set (CMAKE_CXX_STANDARD "11" CACHE STRING "C++ Standard (needs at least 11)")
mark_as_advanced(CMAKE_CXX_STANDARD)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake" ${CMAKE_MODULE_PATH})

set(CMAKE_SKIP_BUILD_RPATH  FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")

find_package(OpenCL REQUIRED)

find_path(VIENNACL_INC_DIR viennacl/matrix.hpp PATHS ${ViennaCL_DIR} ${ViennaCL_DIR}/..)
if (${VIENNACL_INC_DIR} STREQUAL "VIENNACL_INC_DIR-NOTFOUND")
  message(SEND_ERROR "ViennaCL not found!")
endif ()

find_package(TBB REQUIRED)
install(FILES $<TARGET_FILE:TBB::tbb> TYPE LIB)

find_package(Boost REQUIRED)

find_package(Eigen3 REQUIRED)

find_package(fmt REQUIRED)

find_package(ITK REQUIRED)
include(${ITK_USE_FILE})

find_package(VTK REQUIRED)
include(${VTK_USE_FILE})

find_package(OpenCV REQUIRED)

add_subdirectory(third_party)

set(JHMR_EXT_LIB_INC_DIRS ${Boost_INCLUDE_DIRS}
                          ${EIGEN3_INCLUDE_DIR}
                          ${FMT_INCLUDE_DIRS}
                          ${ITK_INCLUDE_DIRS}
                          ${OpenCV_INCLUDE_DIRS}
                          ${VTK_INCLUDE_DIRS}
                          ${VIENNACL_INC_DIR})

set(JHMR_EXT_LIBS_TO_LINK OpenCL::OpenCL
                          fmt::fmt
                          ${ITK_LIBRARIES}
                          ${OpenCV_LIBS}
                          TBB::tbb
                          ${VTK_LIBRARIES}
                          cmaes)

include_directories(SYSTEM ${JHMR_EXT_LIB_INC_DIRS})

add_subdirectory(lib)

set(JHMR_EXE_LIBS_TO_LINK ${JHMR_EXT_LIBS_TO_LINK} jhmr)

# Prefix of each application executable filename
set(JHMR_EXE_PREFIX "jhmr-")

add_subdirectory(apps)

