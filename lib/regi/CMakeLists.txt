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
                             jhmrSE3OptVars.cpp
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
                             sim_metrics_2d/jhmrImgSimMetric2DPatchGradNCCOCL.cpp
                             pnp_solvers/jhmrLandmark2D3DRegi.cpp
                             pnp_solvers/jhmrLandmark2D3DRegiReprojDist.cpp
                             pnp_solvers/jhmrLandmark2D3DRegiReprojDistCMAES.cpp
                             pnp_solvers/jhmrPOSIT.cpp
                             pnp_solvers/jhmrP3PCArm.cpp
                             pnp_solvers/jhmrRANSACPnP.cpp
                             pnp_solvers/jhmrPnPUtils.cpp
                             penalty_fns_2d_3d/jhmrRegi2D3DPenaltyFn.cpp
                             penalty_fns_2d_3d/jhmrRegi2D3DPenaltyFnCombo.cpp
                             penalty_fns_2d_3d/jhmrRegi2D3DPenaltyFnGlobalPelvis.cpp
                             penalty_fns_2d_3d/jhmrRegi2D3DPenaltyFnLandReproj.cpp
                             penalty_fns_2d_3d/jhmrRegi2D3DPenaltyFnPelvisAP.cpp
                             penalty_fns_2d_3d/jhmrRegi2D3DPenaltyFnSE3Mag.cpp
                             penalty_fns_2d_3d/jhmrRegi2D3DPenaltyFnSE3EulerDecomp.cpp
                             penalty_fns_2d_3d/jhmrRegi2D3DPenaltyFnRelPose.cpp
                             penalty_fns_2d_3d/jhmrRegi2D3DPenaltyFnDebugH5.cpp
                             interfaces_2d_3d/jhmrIntensity2D3DRegi.cpp
                             interfaces_2d_3d/jhmrIntensity2D3DRegiDebug.cpp
                             interfaces_2d_3d/jhmrIntensity2D3DRegiCMAES.cpp
                             interfaces_2d_3d/jhmrIntensity2D3DRegiDiffEvo.cpp
                             interfaces_2d_3d/jhmrIntensity2D3DRegiPSO.cpp
                             interfaces_2d_3d/jhmrIntensity2D3DRegiExhaustive.cpp
                             interfaces_2d_3d/jhmrIntensity2D3DRegiHillClimb.cpp
                             interfaces_2d_3d/jhmrIntensity2D3DRegiNLOptInterface.cpp
                             interfaces_2d_3d/jhmrIntensity2D3DRegiBOBYQA.cpp
                             interfaces_2d_3d/jhmrIntensity2D3DRegiCRS.cpp
                             interfaces_2d_3d/jhmrIntensity2D3DRegiDIRECT.cpp
                             interfaces_2d_3d/jhmrIntensity2D3DRegiDESCH.cpp
                             interfaces_2d_3d/jhmrIntensity2D3DRegiDISRES.cpp
                             interfaces_2d_3d/jhmrIntensity2D3DRegiNEWUOA.cpp
                             interfaces_2d_3d/jhmrIntensity2D3DRegiNelderMead.cpp
                             interfaces_2d_3d/jhmrIntensity2D3DRegiPRAXIS.cpp
                             interfaces_2d_3d/jhmrIntensity2D3DRegiSbplx.cpp
                             interfaces_2d_3d/jhmrMultiObjMultiLevel2D3DRegi.cpp
                             interfaces_2d_3d/jhmrMultiObjMultiLevel2D3DRegiDebug.cpp)

target_link_libraries(jhmr_regi PUBLIC ${JHMR_EXT_LIBS_TO_LINK})
target_compile_definitions(jhmr_regi PRIVATE VIENNACL_WITH_OPENCL)

