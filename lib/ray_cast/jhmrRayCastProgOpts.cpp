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

#include "jhmrRayCastProgOpts.h"

#include "jhmrExceptionUtils.h"
#include "jhmrProgOptUtils.h"
#include "jhmrRayCastLineIntCPU.h"
#include "jhmrRayCastLineIntOCL.h"

std::shared_ptr<jhmr::RayCaster>
jhmr::LineIntRayCasterFromProgOpts(const ProgOpts& po)
{
  const std::string backend_str = po.get("backend");

  std::shared_ptr<RayCaster> rc;

  if (backend_str == "cpu")
  {
    rc = std::make_shared<RayCasterLineIntCPU>();
  }
  else if (backend_str == "ocl")
  {
    rc = std::make_shared<RayCasterLineIntOCL>(po.selected_ocl());
  }
  else
  {
    jhmrThrow("Unsupported backend for Line Int. Ray Caster: %s", backend_str.c_str());
  }

  return rc;
}

