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

#include "jhmrIntensity2D3DRegiDebug.h"

void jhmr::SingleRegiDebugResults::init(SE3OptVarsPtr se3, const IndexList& global_vol_inds,
                                        const size_type init_num_iters_capacity)
{
  se3_params = se3;

  vols_used.assign(global_vol_inds.begin(), global_vol_inds.end());

  const size_type num_objs = vols_used.size();

  init_poses.assign(num_objs, FrameTransform::Identity());
  final_poses.assign(num_objs, FrameTransform::Identity());

  inter_frames.assign(num_objs, FrameTransform::Identity());
  inter_frames_wrt_vol.assign(num_objs, false);

  iter_vars.resize(num_objs);
  for (size_type obj_idx = 0; obj_idx < num_objs; ++obj_idx)
  {
    iter_vars[obj_idx].reserve(init_num_iters_capacity);
  }

  sims.reserve(init_num_iters_capacity);
}

