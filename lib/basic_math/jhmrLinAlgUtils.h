
#ifndef JHMRLINALGUTILS_H_
#define JHMRLINALGUTILS_H_

#include "jhmrCommon.h"

namespace jhmr
{

MatMxN ComputePseudoInverse(const MatMxN& a,
                            Eigen::JacobiSVD<MatMxN>* svd_work = nullptr);

}  // jhmr

#endif

