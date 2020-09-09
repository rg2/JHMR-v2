/*
 * MIT License
 *
 * Copyright (c) 2020 Robert Grupp
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "jhmrImgSimMetric2DProgOpts.h"

#include "jhmrExceptionUtils.h"
#include "jhmrProgOptUtils.h"
#include "jhmrImgSimMetric2DSSDCPU.h"
#include "jhmrImgSimMetric2DSSDOCL.h"
#include "jhmrImgSimMetric2DNCCCPU.h"
#include "jhmrImgSimMetric2DNCCOCL.h"
#include "jhmrImgSimMetric2DGradNCCCPU.h"
#include "jhmrImgSimMetric2DGradNCCOCL.h"
#include "jhmrImgSimMetric2DPatchNCCCPU.h"
#include "jhmrImgSimMetric2DPatchNCCOCL.h"
#include "jhmrImgSimMetric2DPatchGradNCCCPU.h"
#include "jhmrImgSimMetric2DPatchGradNCCOCL.h"

namespace  // un-named
{

using namespace jhmr;

template <class tCPUSim, class tOCLSim>
std::shared_ptr<ImgSimMetric2D> SimMetricFromProgOptsHelper(ProgOpts& po)
{
  const std::string backend_str = po.get("backend");
  
  std::shared_ptr<ImgSimMetric2D> sm;

  if (backend_str == "cpu")
  {
    sm = std::make_shared<tCPUSim>();
  }
  else if (backend_str == "ocl")
  {
    auto ocl_ctx_queue = po.selected_ocl_ctx_queue();
    sm = std::make_shared<tOCLSim>(std::get<0>(ocl_ctx_queue), std::get<1>(ocl_ctx_queue));
  }
  else
  {
    jhmrThrow("Unsupported backend for Sim Metric: %s", backend_str.c_str());
  }
  
  return sm;
}

}  // un-named

std::shared_ptr<jhmr::ImgSimMetric2D> jhmr::SSDSimMetricFromProgOpts(ProgOpts& po)
{
  return SimMetricFromProgOptsHelper<ImgSimMetric2DSSDCPU,ImgSimMetric2DSSDOCL>(po);
}

std::shared_ptr<jhmr::ImgSimMetric2D> jhmr::NCCSimMetricFromProgOpts(ProgOpts& po)
{
  return SimMetricFromProgOptsHelper<ImgSimMetric2DNCCCPU,ImgSimMetric2DNCCOCL>(po);
}

std::shared_ptr<jhmr::ImgSimMetric2D> jhmr::GradNCCSimMetricFromProgOpts(ProgOpts& po)
{
  return SimMetricFromProgOptsHelper<ImgSimMetric2DGradNCCCPU,ImgSimMetric2DGradNCCOCL>(po);
}

std::shared_ptr<jhmr::ImgSimMetric2D> jhmr::PatchNCCSimMetricFromProgOpts(ProgOpts& po)
{
  return SimMetricFromProgOptsHelper<ImgSimMetric2DPatchNCCCPU,ImgSimMetric2DPatchNCCOCL>(po);
}

std::shared_ptr<jhmr::ImgSimMetric2D> jhmr::PatchGradNCCSimMetricFromProgOpts(ProgOpts& po)
{
  return SimMetricFromProgOptsHelper<ImgSimMetric2DPatchGradNCCCPU,ImgSimMetric2DPatchGradNCCOCL>(po);
}
