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

add_library(jhmr_file_formats OBJECT jhmrDICOMUtils.cpp
                                     jhmrCSVUtils.cpp
                                     jhmrCIOSFusionDICOM.cpp
                                     jhmrACSVUtils.cpp
                                     jhmrFCSVUtils.cpp
                                     jhmrSTLMeshIO.cpp
                                     jhmrH5MeshIO.cpp
                                     jhmrMeshIO.cpp
                                     jhmrH5PAOIO.cpp
                                     jhmrPAOIO.cpp
                                     jhmrH5CamModelIO.cpp
                                     jhmrH5ProjDataIO.cpp
                                     jhmrH5SE3OptVarsIO.cpp
                                     jhmrWriteVideo.cpp)

target_link_libraries(jhmr_file_formats PUBLIC ${JHMR_EXT_LIBS_TO_LINK})

