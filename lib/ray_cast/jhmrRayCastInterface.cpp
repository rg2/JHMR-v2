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

#include "jhmrRayCastInterface.h"

#include "jhmrAssert.h"
#include "jhmrTBBUtils.h"

void jhmr::RayCaster::set_volume(VolPtr img_vol)
{
  vols_ = { img_vol };

  vols_changed();
}

void jhmr::RayCaster::set_volumes(const VolList& vols)
{
  vols_ = vols;
  
  vols_changed();
}

jhmr::size_type jhmr::RayCaster::num_vols() const
{
  return vols_.size();
}
  
jhmr::size_type jhmr::RayCaster::num_camera_models() const
{
  return camera_models_.size();
}

void jhmr::RayCaster::set_camera_models(const CameraModelList& camera_models)
{
  camera_models_ = camera_models;
  camera_models_changed();
}

void jhmr::RayCaster::set_camera_model(const CameraModel& camera_model)
{
  camera_models_ = { camera_model };
  camera_models_changed();
}

const jhmr::RayCaster::CameraModelList&
jhmr::RayCaster::camera_models() const
{
  return camera_models_;
}

const jhmr::CameraModel&
jhmr::RayCaster::camera_model(const size_type cam_idx) const
{
  return camera_models_[cam_idx];
}

void jhmr::RayCaster::set_proj_cam_model(const size_type proj_idx, const size_type cam_idx)
{
  cam_model_for_proj_[proj_idx] = cam_idx;
}

const jhmr::RayCaster::CamModelAssocList&
jhmr::RayCaster::camera_model_proj_associations() const
{
  return cam_model_for_proj_;
}
  
void jhmr::RayCaster::set_camera_model_proj_associations(const CamModelAssocList& cam_model_for_proj)
{
  jhmrASSERT(cam_model_for_proj.size() == num_projs_);
  // TODO: also check validity of camera indices

  cam_model_for_proj_ = cam_model_for_proj;
}

void jhmr::RayCaster::distribute_xforms_among_cam_models(const FrameTransformList& xforms_cam_to_itk_phys)
{
  const size_type num_passed_xforms = xforms_cam_to_itk_phys.size();

  const size_type num_cams = num_camera_models();

  jhmrASSERT((num_passed_xforms * num_cams) == num_projs());

  size_type global_proj_idx = 0;
  for (size_type cam_idx = 0; cam_idx < num_cams; ++cam_idx)
  {
    for (size_type passed_proj_idx = 0; passed_proj_idx < num_passed_xforms; ++passed_proj_idx, ++global_proj_idx)
    {
      xforms_cam_to_itk_phys_[global_proj_idx] = xforms_cam_to_itk_phys[passed_proj_idx];
      cam_model_for_proj_[global_proj_idx] = cam_idx;
    }
  }
}

void jhmr::RayCaster::distribute_xform_among_cam_models(const FrameTransform& xform_cam_to_itk_phys)
{
  distribute_xforms_among_cam_models(FrameTransformList(1, xform_cam_to_itk_phys));
}

void jhmr::RayCaster::set_ray_step_size(const CoordScalar& step_size)
{
  ray_step_size_ = step_size;
}

jhmr::CoordScalar jhmr::RayCaster::ray_step_size() const
{
  return ray_step_size_;
}

void jhmr::RayCaster::set_num_projs(const size_type num_projs)
{
  // verify that we have either have not allocated resources or can fit the
  // requested number of projections
  jhmrASSERT(!max_num_projs_ || (num_projs_ <= max_num_projs_));
  num_projs_ = num_projs;

  cam_model_for_proj_.resize(num_projs_);
}
  
jhmr::size_type jhmr::RayCaster::num_projs() const
{
  return num_projs_;
}

void jhmr::RayCaster::set_interp_method(const InterpMethod& interp_method)
{
  interp_method_ = interp_method;
}
  
jhmr::RayCaster::InterpMethod jhmr::RayCaster::interp_method() const
{
  return interp_method_;
}

void jhmr::RayCaster::use_linear_interp()
{
  interp_method_ = kRAY_CAST_INTERP_LINEAR;
}

void jhmr::RayCaster::use_nn_interp()
{
  interp_method_ = kRAY_CAST_INTERP_NN;
}

void jhmr::RayCaster::use_sinc_interp()
{
  interp_method_ = kRAY_CAST_INTERP_SINC;
}

void jhmr::RayCaster::use_bspline_interp()
{
  interp_method_ = kRAY_CAST_INTERP_BSPLINE;
}

void jhmr::RayCaster::set_xforms_cam_to_itk_phys(const FrameTransformList& xforms)
{
  set_num_projs(xforms.size());
  xforms_cam_to_itk_phys_ = xforms;
}
  
const jhmr::FrameTransformList&
jhmr::RayCaster::xforms_cam_to_itk_phys() const
{
  return xforms_cam_to_itk_phys_;
}

jhmr::FrameTransform&
jhmr::RayCaster::xform_cam_to_itk_phys(const size_type proj_idx)
{
  return xforms_cam_to_itk_phys_[proj_idx];
}

const jhmr::FrameTransform&
jhmr::RayCaster::xform_cam_to_itk_phys(const size_type proj_idx) const
{
  return xforms_cam_to_itk_phys_[proj_idx];
}

jhmr::FrameTransform
jhmr::RayCaster::xform_img_center_to_itk_phys(const size_type vol_idx) const
{
  // we want to treat image coordinates as having the origin at the volume center,
  // ITK physical coordinates are typically have the origin at the "top left" of the
  // image/volume (or the zero index)

  const auto vol_size = vols_[vol_idx]->GetLargestPossibleRegion().GetSize();

  itk::ContinuousIndex<double,3> vol_center_ind;
  vol_center_ind[0] = vol_size[0] / 2.0;
  vol_center_ind[1] = vol_size[1] / 2.0;
  vol_center_ind[2] = vol_size[2] / 2.0;

  // retrieve the ITK physical point coordinate of the volume center
  itk::Point<double,3> vol_center_pt_wrt_itk;
  vols_[vol_idx]->TransformContinuousIndexToPhysicalPoint(vol_center_ind, vol_center_pt_wrt_itk);

  FrameTransform xform = FrameTransform::Identity();
  xform.matrix()(0,3) = vol_center_pt_wrt_itk[0];
  xform.matrix()(1,3) = vol_center_pt_wrt_itk[1];
  xform.matrix()(2,3) = vol_center_pt_wrt_itk[2];

  return xform;
}

jhmr::FrameTransform
jhmr::RayCaster::xform_cam_wrt_carm_center_of_rot(const size_type cam_idx) const
{
  FrameTransform xform = FrameTransform::Identity();
  xform.matrix()(2,3) -= camera_models_[cam_idx].focal_len / 2;

  return xform;
}

void jhmr::RayCaster::post_multiply_all_xforms(const FrameTransform& post_xform)
{
  auto post_mult_fn = [&] (const RangeType& r)
  {
    for (size_type range_idx = r.begin(); range_idx < r.end(); ++range_idx)
    {
      FrameTransform& cur_frame = xforms_cam_to_itk_phys_[range_idx];
      cur_frame = cur_frame * post_xform;
    }
  };

  ParallelFor(post_mult_fn, RangeType(0, num_projs_));
}

void jhmr::RayCaster::pre_multiply_all_xforms(const FrameTransform& pre_xform)
{
  auto pre_mult_fn = [&] (const RangeType& r)
  {
    for (size_type range_idx = r.begin(); range_idx < r.end(); ++range_idx)
    {
      FrameTransform& cur_frame = xforms_cam_to_itk_phys_[range_idx];
      cur_frame = pre_xform * cur_frame;
    }
  };

  ParallelFor(pre_mult_fn, RangeType(0, num_projs_));
}

void jhmr::RayCaster::allocate_resources()
{
  jhmrASSERT(num_projs_);
  jhmrASSERT(!camera_models_.empty());

  max_num_projs_ = num_projs_;

  xforms_cam_to_itk_phys_.resize(num_projs_);

  cam_model_for_proj_.resize(num_projs_, 0);  // default camera index is 0

  // NOTE: for the gpu, this would be where space would be allocated and the volume memcpy'd

  resources_allocated_ = true;
}

void jhmr::RayCaster::set_proj_store_method(const ProjPixelStoreMethod m)
{
  proj_store_meth_ = m;
}

jhmr::RayCaster::ProjPixelStoreMethod jhmr::RayCaster::proj_store_method() const
{
  return proj_store_meth_;
}

void jhmr::RayCaster::use_proj_store_replace_method()
{
  proj_store_meth_ = kRAY_CAST_PIXEL_REPLACE;
}

void jhmr::RayCaster::use_proj_store_accum_method()
{
  proj_store_meth_ = kRAY_CAST_PIXEL_ACCUM;
}
  
jhmr::RayCastSyncOCLBuf* jhmr::RayCaster::to_ocl_buf()
{
  throw UnsupportedOperationException();
}

jhmr::RayCastSyncHostBuf* jhmr::RayCaster::to_host_buf()
{
  throw UnsupportedOperationException();
}

void jhmr::RayCaster::set_use_bg_projs(const bool use_bg_projs)
{
  use_bg_projs_ = use_bg_projs;
}

bool jhmr::RayCaster::use_bg_projs() const
{
  return use_bg_projs_;
}

void jhmr::RayCaster::set_bg_proj(ProjPtr& proj, const bool use_bg_projs)
{
  bg_projs_for_each_cam_.assign(num_camera_models(), proj);
  use_bg_projs_ = use_bg_projs;

  bg_projs_updated_ = true;
}

void jhmr::RayCaster::set_bg_projs(ProjList& projs, const bool use_bg_projs)
{
  jhmrASSERT(num_camera_models() == projs.size());

  bg_projs_for_each_cam_ = projs;
  use_bg_projs_ = use_bg_projs;

  bg_projs_updated_ = true;
}

jhmr::size_type jhmr::RayCaster::max_num_projs() const
{
  return max_num_projs_;
}

jhmr::RayCaster::PixelScalar2D jhmr::RayCaster::default_bg_pixel_val() const
{
  return default_bg_pixel_val_;
}

void jhmr::RayCaster::set_default_bg_pixel_val(const PixelScalar2D bg_val)
{
  default_bg_pixel_val_ = bg_val;
}
  
void jhmr::RayCasterCollisionParamInterface::set_render_thresh(const PixelScalar t)
{
  collision_params_.thresh = t;
}

void jhmr::RayCasterCollisionParamInterface::set_num_backtracking_steps(const size_type num_steps)
{
  collision_params_.num_backtracking_steps = num_steps;
}

jhmr::RayCasterCollisionParamInterface::PixelScalar
jhmr::RayCasterCollisionParamInterface::render_thresh() const
{
  return collision_params_.thresh;
}

jhmr::size_type jhmr::RayCasterCollisionParamInterface::num_backtracking_steps() const
{
  return collision_params_.num_backtracking_steps;
}

const jhmr::RayCasterCollisionParams&
jhmr::RayCasterCollisionParamInterface::collision_params() const
{
  return collision_params_;
}
  
jhmr::RayCasterSurRenderParamInterface::RayCasterSurRenderParamInterface()
{
  set_render_thresh(200);
  set_num_backtracking_steps(20);

  sur_render_params_.ambient_reflection_ratio  = 0.25;
  sur_render_params_.diffuse_reflection_ratio  = 0.7;
  sur_render_params_.specular_reflection_ratio = 0.05;
  sur_render_params_.alpha_shininess           = 1.0;
}

void jhmr::RayCasterSurRenderParamInterface::set_sur_render_params(
                      const RayCasterSurRenderShadingParams& sur_render_params)
{
  sur_render_params_ = sur_render_params;
}

const jhmr::RayCasterSurRenderShadingParams&
jhmr::RayCasterSurRenderParamInterface::surface_render_params() const
{
  return sur_render_params_;
}

void jhmr::RayCasterSurRenderParamInterface::set_ambient_reflection_ratio(
                                              const PixelScalar amb_ref_ratio)
{
  sur_render_params_.ambient_reflection_ratio = amb_ref_ratio;
}

void jhmr::RayCasterSurRenderParamInterface::set_diffuse_reflection_ratio(
                                              const PixelScalar diff_ref_ratio)
{
  sur_render_params_.diffuse_reflection_ratio = diff_ref_ratio;
}

void jhmr::RayCasterSurRenderParamInterface::set_specular_reflection_ratio(
                                              const PixelScalar spec_ref_ratio)
{
  sur_render_params_.specular_reflection_ratio = spec_ref_ratio;
}

void jhmr::RayCasterSurRenderParamInterface::set_alpha_shininess(const PixelScalar alpha)
{
  sur_render_params_.alpha_shininess = alpha;
}
  
jhmr::RayCasterOccludingContours::RayCasterOccludingContours()
{
  this->set_render_thresh(150);
  this->set_num_backtracking_steps(0);
}

jhmr::CoordScalar jhmr::RayCasterOccludingContours::occlusion_angle_thresh_rad() const
{
  return occlusion_angle_thresh_rad_;
}

void jhmr::RayCasterOccludingContours::set_occlusion_angle_thresh_rad(const CoordScalar ang_rad)
{
  occlusion_angle_thresh_rad_ = ang_rad;
}

void jhmr::RayCasterOccludingContours::set_occlusion_angle_thresh_deg(const CoordScalar ang_deg)
{
  occlusion_angle_thresh_rad_ = ang_deg * kDEG2RAD;
}

bool jhmr::RayCasterOccludingContours::stop_after_collision() const
{
  return stop_after_collision_;
}

void jhmr::RayCasterOccludingContours::set_stop_after_collision(const bool stop_after_coll)
{
  stop_after_collision_ = stop_after_coll;
}

void jhmr::SimpleRayCasterWrapperFn::operator()()
{
  const bool vols_to_proj_specified = !vols_to_proj.empty();

  const size_type nv = vols_to_proj_specified ? vols_to_proj.size() :
                                                ray_caster->num_vols();

  jhmrASSERT(cam_world_to_vols.size() == nv);

  const bool inter_frames_specified = !inter_frames.empty();
  jhmrASSERT(!inter_frames_specified ||
             ((inter_frames.size() == nv) &&
              (nv == inter_frames_wrt_vol.size()) &&
              (nv == ref_frames_cam_world_to_vol.size())));

  const size_type num_cams = ray_caster->num_camera_models();

  projs.resize(num_cams);

  // save off some state from the ray caster
  const size_type prev_num_projs = ray_caster->num_projs();
  const auto cam_assocs = ray_caster->camera_model_proj_associations();

  // now, we'll create a projection for each view/camera
  ray_caster->set_num_projs(num_cams);

  // Perform the ray casting of multiple, or one, object(s), by looping over
  // each volume, setting the pose parameters for the current volume, and
  // ray casting for the current volume. The projection buffers are initialized
  // for the first projection and then accumulated afterwards.
  for (size_type vol_idx = 0; vol_idx < nv; ++vol_idx)
  {
    const size_type ray_caster_vol_idx = vols_to_proj_specified ?
                                                      vols_to_proj[vol_idx] :
                                                      vol_idx;
    jhmrASSERT(ray_caster_vol_idx < ray_caster->num_vols());

    // TODO: have another case when replacing with another image
    if (vol_idx != 0)
    {
      ray_caster->use_proj_store_accum_method();
    }
    else
    {
      ray_caster->use_proj_store_replace_method();
    }

    ray_caster->distribute_xform_among_cam_models(cam_world_to_vols[vol_idx]);

    if (inter_frames_specified)
    {
      if (inter_frames_wrt_vol[vol_idx])
      {
        ray_caster->pre_multiply_all_xforms(inter_frames[vol_idx]);
        ray_caster->post_multiply_all_xforms(inter_frames[vol_idx].inverse() *
                                             ref_frames_cam_world_to_vol[vol_idx]);
      }
      else
      {
        ray_caster->pre_multiply_all_xforms(ref_frames_cam_world_to_vol[vol_idx] *
                                            inter_frames[vol_idx]);
        ray_caster->post_multiply_all_xforms(inter_frames[vol_idx].inverse());
      }
    }

    ray_caster->compute(ray_caster_vol_idx);
  }

  // copy the projections
  for (size_type cam_idx = 0; cam_idx < num_cams; ++cam_idx)
  {
    projs[cam_idx] = ray_caster->proj(cam_idx);
  }

  // restore the previous ray caster state
  ray_caster->set_num_projs(prev_num_projs);
  ray_caster->set_camera_model_proj_associations(cam_assocs); 
}

jhmr::RayCastLineIntKernel jhmr::RayCastLineIntParamInterface::kernel_id() const
{
  return kernel_id_;
}

void jhmr::RayCastLineIntParamInterface::set_kernel_id(const RayCastLineIntKernel k)
{
  kernel_id_ = k;
}

