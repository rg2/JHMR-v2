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

cmake_minimum_required(VERSION 3.13.0 FATAL_ERROR)

# Version format is YYYY.MM.DD for a release and YYYY.MM.DD.1 for devel after the release
project(jhmr-v2 VERSION 2020.0.0.1)

set (CMAKE_CXX_STANDARD "11" CACHE STRING "C++ Standard (needs at least 11)")
mark_as_advanced(CMAKE_CXX_STANDARD)

set (JHMR_TOP_LEVEL_PROJ FALSE)
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  set (JHMR_TOP_LEVEL_PROJ TRUE)
endif ()

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake" ${CMAKE_MODULE_PATH})

option(JHMR_BUILD_APPS "Build jhmr applications in addition to the library" ${JHMR_TOP_LEVEL_PROJ})

option(JHMR_INCLUDE_GIT_HASH_IN_VER_STR "Include the git hash in the version string that is embedded into the library" OFF)
mark_as_advanced(JHMR_INCLUDE_GIT_HASH_IN_VER_STR)

set(CMAKE_SKIP_BUILD_RPATH  FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")

find_package(OpenCL REQUIRED)

find_path(VIENNACL_INC_DIR viennacl/matrix.hpp PATHS ${ViennaCL_DIR} ${ViennaCL_DIR}/..)
if (${VIENNACL_INC_DIR} STREQUAL "VIENNACL_INC_DIR-NOTFOUND")
  message(SEND_ERROR "ViennaCL not found!")
endif ()

find_package(TBB REQUIRED)
install(FILES $<TARGET_FILE:tbb> TYPE LIB)

find_package(Boost REQUIRED)

find_package(Eigen3 REQUIRED)

find_package(fmt REQUIRED)

find_package(ITK REQUIRED)
include(${ITK_USE_FILE})

find_package(VTK REQUIRED)
include(${VTK_USE_FILE})

find_package(OpenCV REQUIRED)

find_package(NLopt REQUIRED)

add_subdirectory(third_party)

set(JHMR_EXT_LIB_INC_DIRS ${Boost_INCLUDE_DIRS}
                          ${EIGEN3_INCLUDE_DIR}
                          ${FMT_INCLUDE_DIRS}
                          ${ITK_INCLUDE_DIRS}
                          ${ITK_INCLUDE_DIRS}/itkhdf5
                          ${OpenCV_INCLUDE_DIRS}
                          ${VTK_INCLUDE_DIRS}
                          ${VIENNACL_INC_DIR})

set(JHMR_EXT_LIBS_TO_LINK OpenCL::OpenCL
                          fmt::fmt
                          ${ITK_LIBRARIES}
                          ${OpenCV_LIBS}
                          tbb
                          ${VTK_LIBRARIES}
                          ${NLOPT_LIBRARIES})

include_directories(SYSTEM ${JHMR_EXT_LIB_INC_DIRS})

if (JHMR_INCLUDE_GIT_HASH_IN_VER_STR)
  include(GetGitRevisionDescription)
  get_git_head_revision(GIT_REFSPEC JHMR_GIT_SHA1)
  set(JHMR_HAS_GIT_SHA1 true)
else ()
  set(JHMR_GIT_SHA1 "")
  set(JHMR_HAS_GIT_SHA1 false)
endif ()

add_subdirectory(lib)

set(JHMR_EXE_LIBS_TO_LINK ${JHMR_EXT_LIBS_TO_LINK} jhmr)

# Prefix of each application executable filename
set(JHMR_EXE_PREFIX "jhmr-")

if (JHMR_BUILD_APPS)
  add_subdirectory(apps)
endif ()

# only install targets and config when building the library
# as its own top-level project. This is needed for when the
# library file tree is located within another project and added
# via a call to add_subdirectory()
if (JHMR_TOP_LEVEL_PROJ)
  install(TARGETS jhmr cmaes EXPORT jhmr-targets)

  include(CMakePackageConfigHelpers)

  configure_package_config_file(cmake/jhmrConfig.cmake.in
                                ${PROJECT_BINARY_DIR}/jhmrConfig.cmake
                                INSTALL_DESTINATION lib/cmake)

  write_basic_package_version_file(${PROJECT_BINARY_DIR}/jhmrConfigVersion.cmake COMPATIBILITY AnyNewerVersion)

  # Even though the cmaes target may be static library with a private
  # relationship to jhmr, CMake still requires it to be exported!
  export(TARGETS jhmr cmaes NAMESPACE jhmr:: FILE ${PROJECT_BINARY_DIR}/jhmr-targets.cmake)

  install(FILES ${PROJECT_BINARY_DIR}/jhmrConfig.cmake ${PROJECT_BINARY_DIR}/jhmrConfigVersion.cmake
          DESTINATION lib/cmake)

  install(EXPORT jhmr-targets NAMESPACE jhmr:: DESTINATION lib/cmake)
endif ()

