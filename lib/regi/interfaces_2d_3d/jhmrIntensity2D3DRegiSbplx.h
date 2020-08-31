
#ifndef JHMRINTENSITY2D3DREGISBPLX_H_
#define JHMRINTENSITY2D3DREGISBPLX_H_

#include "jhmrIntensity2D3DRegiNLOptInterface.h"

namespace jhmr
{

/// \brief 2D/3D Intensity-Based Registration object using the Sbplx (based on Subplex) optimization method.
///
/// This is an unconstrained optimization algorithm, but allows bounds to be specified.
class Intensity2D3DRegiSbplx : public Intensity2D3DRegiNLOptInterface
{
public:
  Intensity2D3DRegiSbplx();
};

}  // jhmr

#endif
