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



#ifndef JHMRBASICSTATS_H_
#define JHMRBASICSTATS_H_

#include "jhmrCommon.h"

namespace jhmr
{

CoordScalar SampleMean(const CoordScalarList& x);

CoordScalar SampleStdDev(const CoordScalarList& x);

// Computes the sample standard deviation, but saves some computation by using
// a previously computed value for the sample mean
CoordScalar SampleStdDev(const CoordScalarList& x, const CoordScalar sample_mean);

// Computes the sample mean and sample standard deviation, returned as a tuple (mean, std. dev.)
std::tuple<CoordScalar,CoordScalar> SampleMeanAndStdDev(const CoordScalarList& x);

}  // jhmr

#endif

