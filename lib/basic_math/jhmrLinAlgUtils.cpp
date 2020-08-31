
#include "jhmrLinAlgUtils.h"

jhmr::MatMxN jhmr::ComputePseudoInverse(const MatMxN& a,
                                        Eigen::JacobiSVD<MatMxN>* svd_work)
{
  constexpr CoordScalar eps = std::numeric_limits<CoordScalar>::epsilon();
  
  // Adapted from: http://eigen.tuxfamily.org/bz/show_bug.cgi?id=257

  Eigen::JacobiSVD<MatMxN> local_svd;

  Eigen::JacobiSVD<MatMxN>* svd = svd_work;
  if (!svd)
  {
    svd = &local_svd;
  }

  svd->compute(a, Eigen::ComputeThinU | Eigen::ComputeThinV);

  const CoordScalar tol = eps * std::max(a.cols(), a.rows()) * svd->singularValues().array().abs().maxCoeff();

  return svd->matrixV() *
         MatMxN((svd->singularValues().array().abs() > tol).select(
                     svd->singularValues().array().inverse(), 0) ).asDiagonal() *
         svd->matrixU().adjoint();
}

