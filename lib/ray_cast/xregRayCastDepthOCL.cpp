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

#include "xregRayCastDepthOCL.h"

#include <boost/compute/utility/source.hpp>
#include <boost/compute/types/struct.hpp>

#include "xregAssert.h"
#include "xregExceptionUtils.h"
#include "xregITKBasicImageUtils.h"
#include "xregOpenCLConvert.h"

namespace bc = boost::compute;

//////////////////////////////////////////////////////////////////////

struct SurCollArgs
{
  bc::float16_ itk_idx_to_itk_phys_pt_xform;
  
  bc::ulong_ num_backtracking_steps;
  
  bc::float_ sur_coll_thresh;

  bc::float_ pad1;
};

BOOST_COMPUTE_ADAPT_STRUCT(SurCollArgs, SurCollArgs,
                           (itk_idx_to_itk_phys_pt_xform, num_backtracking_steps, sur_coll_thresh, pad1))

//////////////////////////////////////////////////////////////////////

namespace  // un-named
{

const char* kRAY_CASTING_DEPTH_OPENCL_SRC = BOOST_COMPUTE_STRINGIZE_SOURCE(

__kernel void xregDepthKernel(const RayCastArgs args,
                              const SurCollArgs sur_coll_args,
                              __global const float4* det_pts,
                              image3d_t vol_tex,
                              __global const float16* cam_to_itk_phys_xforms,
                              __global float* dst_depth_buf,
                              __global const ulong* cam_model_for_proj,
                              __global const float4* cam_focal_pts)
{
  const ulong idx = get_global_id(0);

  const ulong num_rays = args.num_projs * args.num_det_pts;

  if (idx < num_rays)
  {
    const ulong proj_idx   = idx / args.num_det_pts;
    const ulong det_pt_idx = idx - (proj_idx * args.num_det_pts);
    const ulong cam_idx    = cam_model_for_proj[proj_idx];

    const float4 focal_pt_wrt_cam = cam_focal_pts[cam_idx];

    const sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP | CLK_FILTER_LINEAR;

    const float16 xform_cam_to_itk_idx = xregFrm4x4Composition(args.itk_phys_pt_to_itk_idx_xform, cam_to_itk_phys_xforms[proj_idx]);

    const float16 xform_itk_idx_to_cam = xregFrm4x4Composition(xregFrm4x4SE3Inv(cam_to_itk_phys_xforms[proj_idx]),
                                                               sur_coll_args.itk_idx_to_itk_phys_pt_xform);

    const float4 pinhole_wrt_itk_idx = xregFrm4x4XformFloat4Pt(xform_cam_to_itk_idx, focal_pt_wrt_cam);

    const float4 cur_det_pt_wrt_cam = det_pts[(cam_idx * args.num_det_pts) + det_pt_idx];

    const float4 pinhole_to_det_wrt_itk_idx = xregFrm4x4XformFloat4Pt(xform_cam_to_itk_idx, cur_det_pt_wrt_cam) - pinhole_wrt_itk_idx;

    // check intersection with the image/volume boundary
    // these pointer were passed as float4's since boost::compute does not have a float3
    float2 t = xregRayRectIntersect(xregFloat4HmgToFloat3(args.img_aabb_min),
                                    xregFloat4HmgToFloat3(args.img_aabb_max),
                                    xregFloat4HmgToFloat3(pinhole_wrt_itk_idx),
                                    xregFloat4HmgToFloat3(pinhole_to_det_wrt_itk_idx));
    // t.x indicates the first intersection with the volume along the source to detector ray,
    // t.y indicates the exit of the volume along the source to detector ray
    // t.x, t.y are in [0,inf)

    const float4 start_pt_wrt_itk_idx = pinhole_wrt_itk_idx + (t.x * pinhole_to_det_wrt_itk_idx);

    const float pinhole_to_det_len_wrt_itk_idx = xregFloat4HmgNorm(pinhole_to_det_wrt_itk_idx);
    const float intersect_len_wrt_itk_idx = (t.y - t.x) * pinhole_to_det_len_wrt_itk_idx;

    const float4 focal_pt_to_det_wrt_cam = cur_det_pt_wrt_cam - focal_pt_wrt_cam;
    // NOTE: since we are transforming a vector with a scaling transform (points to indices),
    //       the output vector may not have the same norm, thus we take the norm
    const float step_len_wrt_itk_idx = xregFloat4HmgNorm(
                                          xregFrm4x4XformFloat4Vec(xform_cam_to_itk_idx,
                                                                   (focal_pt_to_det_wrt_cam / xregFloat4HmgNorm(focal_pt_to_det_wrt_cam)) * args.step_size));

    const ulong num_steps = (ulong)(intersect_len_wrt_itk_idx / step_len_wrt_itk_idx);

    float4 cur_cont_vol_idx = (float4) (start_pt_wrt_itk_idx.x, start_pt_wrt_itk_idx.y, start_pt_wrt_itk_idx.z, 0);

    // "/ pinhole_to_det_len_wrt_itk_idx" makes pinhole_to_det_wrt_itk_idx a unit vector
    const float scale_to_step = step_len_wrt_itk_idx / pinhole_to_det_len_wrt_itk_idx;

    const float4 step_vec_wrt_itk_idx = (float4) (pinhole_to_det_wrt_itk_idx.x * scale_to_step, pinhole_to_det_wrt_itk_idx.y * scale_to_step, pinhole_to_det_wrt_itk_idx.z * scale_to_step, 0);

    for (ulong step_idx = 0; step_idx <= num_steps; ++step_idx, cur_cont_vol_idx += step_vec_wrt_itk_idx)
    {
      if (read_imagef(vol_tex, sampler, cur_cont_vol_idx).x >= sur_coll_args.sur_coll_thresh)
      {
        cur_cont_vol_idx.x;
        cur_cont_vol_idx.y;
        cur_cont_vol_idx.z;

        const float3 coll_pt_wrt_cam = xregFrm4x4XformFloat3Vec(xform_itk_idx_to_cam,
                                                                xregFloat4HmgToFloat3(cur_cont_vol_idx));

        const float depth = xregFloat3Norm(xregFloat4HmgToFloat3(focal_pt_wrt_cam) - coll_pt_wrt_cam);

        dst_depth_buf[idx] = min(dst_depth_buf[idx], depth);

        break;
      }
    }

  }  // end if (idx < num_rays)
}

);

//////////////////////////////////////////////////////////////////////

}  // un-named

xreg::RayCasterDepthOCL::RayCasterDepthOCL()
  : RayCasterOCL()
{
  this->set_default_bg_pixel_val(std::numeric_limits<PixelScalar2D>::max());
}

xreg::RayCasterDepthOCL::RayCasterDepthOCL(const boost::compute::device& dev)
  : RayCasterOCL(dev)
{
  this->set_default_bg_pixel_val(std::numeric_limits<PixelScalar2D>::max());
}

xreg::RayCasterDepthOCL::RayCasterDepthOCL(const boost::compute::context& ctx,
                                           const boost::compute::command_queue& queue)
  : RayCasterOCL(ctx,queue)
{
  this->set_default_bg_pixel_val(std::numeric_limits<PixelScalar2D>::max());
}

void xreg::RayCasterDepthOCL::compute(const size_type vol_idx)
{
  xregASSERT(this->resources_allocated_);

  compute_helper_pre_kernels(vol_idx);

  // setup kernel arguments and launch
  const FrameTransform itk_idx_to_itk_phys_pt_xform = ITKImagePhysicalPointTransformsAsEigen(this->vols_[vol_idx].GetPointer());
  
  SurCollArgs coll_args = { OpenCLFloat16ToBoostComp16(ConvertToOpenCL(itk_idx_to_itk_phys_pt_xform)),
                            0, this->render_thresh() };

  dev_kernel_.set_arg(0, sizeof(ray_cast_kernel_args_), &ray_cast_kernel_args_);
  
  dev_kernel_.set_arg(1, sizeof(coll_args), &coll_args);

  dev_kernel_.set_arg(2, det_pts_dev_);

  dev_kernel_.set_arg(3, vol_texs_dev_[vol_idx]);

  dev_kernel_.set_arg(4, cam_to_itk_phys_xforms_dev_);
  
  dev_kernel_.set_arg(5, *proj_pixels_dev_to_use_);

  dev_kernel_.set_arg(6, cam_model_for_proj_dev_);

  dev_kernel_.set_arg(7, focal_pts_dev_);

  std::size_t global_work_size = ray_cast_kernel_args_.num_det_pts * this->num_projs_;

  cmd_queue_.enqueue_nd_range_kernel(dev_kernel_,
                                     1, // dim
                                     0, // null offset -> start at 0
                                     &global_work_size,
                                     0  // passing null lets open CL pick a local size
                                    ).wait();

  compute_helper_post_kernels(vol_idx);
}

void xreg::RayCasterDepthOCL::allocate_resources()
{
  if (this->num_backtracking_steps() > 0)
  {
    xregThrow("For OpenCL Depth - Backtracking is NOT supported!");
  }

  RayCasterOCL::allocate_resources();

  // Build the surface rendering ray casting program
  std::stringstream ss;
  ss << RayCastBaseOCLStr()
     << bc::type_definition<SurCollArgs>()
     << kRAY_CASTING_DEPTH_OPENCL_SRC;

  bc::program prog = bc::program::create_with_source(ss.str(), ctx_);

  try
  {
    prog.build();
  }
  catch (bc::opencl_error &e)
  {
    std::cerr << "OpenCL Kernel Compile Error (RayCasterDepthOCL):\n"
              << prog.build_log() << std::endl;
    throw;
  }

  dev_kernel_ = prog.create_kernel("xregDepthKernel");
}

