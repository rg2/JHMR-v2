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

add_library(jhmr_regi OBJECT jhmrPairedPointRegi3D3D.cpp
                             jhmrICP3D3D.cpp
                             sim_metrics_2d/jhmrImgSimMetric2D.cpp
                             sim_metrics_2d/jhmrImgSimMetric2DCombine.cpp
                             sim_metrics_2d/jhmrImgSimMetric2DPatchCommon.cpp
                             sim_metrics_2d/jhmrImgSimMetric2DCPU.cpp
                             sim_metrics_2d/jhmrImgSimMetric2DSSDCPU.cpp
                             sim_metrics_2d/jhmrImgSimMetric2DNCCCPU.cpp
                             sim_metrics_2d/jhmrImgSimMetric2DGradImgCPU.cpp
                             sim_metrics_2d/jhmrImgSimMetric2DGradNCCCPU.cpp
                             sim_metrics_2d/jhmrImgSimMetric2DGradDiffCPU.cpp
                             sim_metrics_2d/jhmrImgSimMetric2DGradOrientCPU.cpp
                             sim_metrics_2d/jhmrImgSimMetric2DBoundaryEdgesCPU.cpp
                             sim_metrics_2d/jhmrImgSimMetric2DPatchNCCCPU.cpp
                             sim_metrics_2d/jhmrImgSimMetric2DPatchGradNCCCPU.cpp
                             sim_metrics_2d/jhmrImgSimMetric2DOCL.cpp
                             sim_metrics_2d/jhmrImgSimMetric2DSSDOCL.cpp
                             sim_metrics_2d/jhmrImgSimMetric2DNCCOCL.cpp
                             sim_metrics_2d/jhmrImgSimMetric2DGradImgOCL.cpp
                             sim_metrics_2d/jhmrImgSimMetric2DGradNCCOCL.cpp
                             sim_metrics_2d/jhmrImgSimMetric2DPatchNCCOCL.cpp
                             sim_metrics_2d/jhmrImgSimMetric2DPatchGradNCCOCL.cpp)

target_link_libraries(jhmr_regi PUBLIC ${JHMR_EXT_LIBS_TO_LINK})
target_compile_definitions(jhmr_regi PRIVATE VIENNACL_WITH_OPENCL)

