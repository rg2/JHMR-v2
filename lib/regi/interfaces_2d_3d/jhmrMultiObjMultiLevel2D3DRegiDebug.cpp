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

#include "jhmrMultiObjMultiLevel2D3DRegiDebug.h"

jhmr::size_type jhmr::DebugRegiResultsMultiLevel::total_num_projs_per_view() const
{
  size_type num_projs = 0;

  const size_type num_levels = regi_results.size();

  for (size_type level_idx = 0; level_idx < num_levels; ++level_idx)
  {
    const size_type num_regis = regi_results[level_idx].size();

    for (size_type regi_idx = 0; regi_idx < num_regis; ++regi_idx)
    {
      num_projs += regi_results[level_idx][regi_idx]->iter_vars[0].size() + 2; // +2 for initial and final
    }
  }

  return num_projs;
}

