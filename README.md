# JHMR-v2
This repository contains library routines and stand-alone programs for various tasks relating to medical image analysis for intraoperative navigation, preoperative planning, and postoperative analysis.
The acronym JHMR was originally derived from this software's original development by Rob Grupp while working with the "Johns Hopkins Models and Registration" research group.
This is the second iteration of the JHMR repository and focuses on minimizing compile times and being reusable to novice developers/researchers.

## Library Features:
* Registration:
  * [Paired Point 3D/3D](lib/regi/jhmrPairedPointRegi3D3D.h)
  * [3D Point Cloud to 3D Surface ICP](lib/regi/jhmrICP3D3D.h)
* Mesh Processing:
  * [Triangular and tetrahedral mesh representations](lib/common/jhmrMesh.h)
* Image/Volume Processing:
  * [Interpolation of non-uniform spaced slices](lib/image/jhmrVariableSpacedSlices.h)
  * Image processing operations leveraging lower-level [ITK](lib/itk) and [OpenCV](lib/opencv/jhmrOpenCVUtils.h) routines
* Numerical Optimization:
  * [Configurable line search implementation](lib/optim/jhmrLineSearchOpt.h)
  * Implementations of several derivative-free methods, including [Differential Evolution](lib/optim/jhmrDiffEvo.h), [Simulated Annealing](lib/optim/jhmrSimAnn.h), and [Particle Swarm Optimization](lib/optim/jhmrPSO.h)
  * [Wrapper around C implementation of CMA-ES optimization](lib/optim/jhmrCMAESInterface.h)
  * [Suite of test objective functions](lib/optim/jhmrOptimTestObjFns.h)
* Geometric Primitives and Spatial Data Structures
  * [KD-Tree for points or surfaces of arbitrary dimension](lib/spatial/jhmrKDTree.h)
  * [Primitives with support for intersection, etc.](lib/spatial/jhmrSpatialPrimitives.h)
  * Fitting primitives to data ([circle](lib/spatial/jhmrFitCircle.h), [plane](lib/spatial/jhmrFitPlane.h)) with robustness to outliers
* Spatial Transformation Utilities:
  * [Rotation](lib/transforms/jhmrRotUtils.h) and [rigid](lib/transforms/jhmrRigidUtils.h) transformation utilities, including lie group/algebra routines
  * [Perspective projection (3D to 2D)](lib/transforms/jhmrPerspectiveXform.h)
  * [Calculation of anatomical coordinate frames](lib/transforms/jhmrAnatCoordFrames.h)
  * [Point cloud manipulation](lib/transforms/jhmrPointCloudUtils.h)
* Visualization
  * [Interactive 3D scene plotting using VTK](lib/vtk/jhmrVTK3DPlotter.h)
* File I/O:
  * [Common DICOM fields](lib/file_formats/jhmrDICOMUtils.h)
  * [DICOM files from Siemens CIOS Fusion C-arm](lib/file_formats/jhmrCIOSFusionDICOM.h)
  * [Helpers for HDF5 reading/writing](lib/hdf5/jhmrHDF5.h)
  * Various mesh formats
  * Various image/volume formats (via ITK)
  * 3D Slicer annotations, [FCSV](lib/file_formats/jhmrFCSVUtils.h) and [ACSV](lib/file_formats/jhmrACSVUtils.h)
  * [Comma separated value (CSV) files](lib/file_formats/jhmrCSVUtils.h)
* Basic Math Utilities:
  * [Basic statistics](lib/basic_math/jhmrBasicStats.h)
  * [Distribution fitting](lib/basic_math/jhmrNormDistFit.h)
* General/Common:
  * [String parsing/manipulation utilities](lib/common/jhmrStringUtils.h)
  * [Serialization streams](lib/common/jhmrStreams.h)
  * [Command line argument parsing](lib/common/jhmrProgOptUtils.h)
  * [Timer class for measuring runtimes with a stopwatch-like interface](lib/common/jhmrTimer.h)
  * [Basic filesystem utilities](lib/common/jhmrFilesystemUtils.h)
  * [Runtime assertions](lib/common/jhmrAssert.h)
* Hip Surgery:
  * [Guessing labels of bones from segmentation volumes](lib/hip_surgery/jhmrHipSegUtils.h)
  * [Planning and modeling of osteotomies](lib/hip_surgery/jhmrPAOCuts.h)
  * [Visualization of osteotomies in 3D](lib/hip_surgery/jhmrPAODrawBones.h)
  * [Modeling of surgical objects, such as screws and K-wires](lib/hip_surgery/jhmrMetalObjs.h)
  * Support for simulated data creation, including [randomized screw and K-wire shapes and poses](lib/hip_surgery/jhmrMetalObjSampling.h), and [volumetric data incorporating osteotomies, repositioned bones, and inserted screws and K-wires](lib/hip_surgery/jhmrPAOVolAfterRepo.h)

## Programs
Some of the capabilities provided by individual programs contained with the apps directory include:
* Image I/O:
  * [DICOM conversion and resampling](apps/image_io/convert_resample_dicom)
  * [Volume cropping](apps/image_io/crop_vol)
  * [Printing DICOM metadata](apps/image_io/report_dicom)
* Mesh processing:
  * [Mesh creation](apps/mesh/create_mesh)
  * [Mesh display](apps/mesh/show_mesh)
* Basic point cloud operations:
  * [Printing FCSV contents](apps/point_clouds/print_fcsv)
  * [Warping FCSV files](apps/point_clouds/xform_fcsv)
* Registration
  * [ICP for 3D point cloud to 3D surface registration](apps/mesh/sur_regi)
* Hip Surgery: Periacetabular Osteotomy (PAO)
  * [Osteotomy planning and modeling](apps/hip_surgery/pao/create_fragment)
  * [Osteotomy 3D visualization](apps/hip_surgery/pao/draw_bones)
  * [Randomized simulation of fragment adjustments](apps/hip_surgery/pao/sample_frag_moves)
  * [Volumetric modeling of fragment adjustments](apps/hip_surgery/pao/create_repo_vol)
  * [Volumetric modeling of fragment fixation using screws and K-wires](apps/hip_surgery/pao/add_screw_kwires_to_vol)

## Planned Work
Although the following capabilities currently only exist in the first iteration of JHMR software, they will be incorporated into this repository at a future date:
* 2D/3D Registration (intensity-based and feature-based)
* Advanced visualization of projective geometry coordinate frames with a scene of 3D objects
* Advanced debugging tools for 2D/3D registration methods
* Creation of simulated fluoroscopy for 2D/3D registration experiments
* Intraoperative reconstruction of PAO bone fragments
* Utilities for creation and manipulation of statistical shape models
* Shape completion from partial shapes and statistical models
* More point cloud manipulation utilities
* Python bindings, conda integration
* And more...

## Dependencies
* C++ 11 compatible compiler
* External libraries (compatible versions are listed):
  * [Intel Threading Building Blocks (TBB)](https://github.com/oneapi-src/oneTBB) (20170919oss)
  * [Boost](https://www.boost.org) (header only) (1.65)
  * [Eigen3](http://eigen.tuxfamily.org) (3.3.4)
  * [fmt](https://fmt.dev) (5.3.0)
  * [ITK](https://itk.org) (4.13.2)
  * [VTK](https://vtk.org) (7.1.1)
  * [OpenCV](https://opencv.org) (3.2.0)

## Building
A standard CMake configure/generate process is used.
It is recommended to generate Ninja build files for fast and efficient compilation. 
<!--An example script for building all dependencies and the JHMR-v2 repository is also provided [here](TODO).-->

## License and Attribution
The software is available for use under the [MIT License](LICENSE).

If you have found this software useful in your work, we kindly ask that you cite the most appropriate references below:
```
Grupp, Robert B., et al. "Pose estimation of periacetabular osteotomy fragments with intraoperative X-ray navigation." IEEE Transactions on Biomedical Engineering 67.2 (2019): 441-452.
----------------------------------------------------------------------
@article{grupp2019pose,
  title={Pose estimation of periacetabular osteotomy fragments with intraoperative {X}-ray navigation},
  author={Grupp, Robert B and Hegeman, Rachel A and Murphy, Ryan J and Alexander, Clayton P and Otake, Yoshito and McArthur, Benjamin A and Armand, Mehran and Taylor, Russell H},
  journal={IEEE Transactions on Biomedical Engineering},
  volume={67},
  number={2},
  pages={441--452},
  year={2019},
  publisher={IEEE}
}
```
```
Grupp, Robert B., Mehran Armand, and Russell H. Taylor. "Patch-based image similarity for intraoperative 2D/3D pelvis registration during periacetabular osteotomy." OR 2.0 Context-Aware Operating Theaters, Computer Assisted Robotic Endoscopy, Clinical Image-Based Procedures, and Skin Image Analysis. Springer, Cham, 2018. 153-163.
----------------------------------------------------------------------
@incollection{grupp2018patch,
  title={Patch-based image similarity for intraoperative {2D}/{3D} pelvis registration during periacetabular osteotomy},
  author={Grupp, Robert B and Armand, Mehran and Taylor, Russell H},
  booktitle={OR 2.0 Context-Aware Operating Theaters, Computer Assisted Robotic Endoscopy, Clinical Image-Based Procedures, and Skin Image Analysis},
  pages={153--163},
  year={2018},
  publisher={Springer}
}
```
```
Grupp, Robert B., et al. "Automatic annotation of hip anatomy in fluoroscopy for robust and efficient 2D/3D registration." International Journal of Computer Assisted Radiology and Surgery (2020): 1-11.
----------------------------------------------------------------------
@article{grupp2020automatic,
  title={Automatic annotation of hip anatomy in fluoroscopy for robust and efficient {2D}/{3D} registration},
  author={Grupp, Robert B and Unberath, Mathias and Gao, Cong and Hegeman, Rachel A and Murphy, Ryan J and Alexander, Clayton P and Otake, Yoshito and McArthur, Benjamin A and Armand, Mehran and Taylor, Russell H},
  journal={International Journal of Computer Assisted Radiology and Surgery},
  pages={1--11},
  publisher={Springer}
}
```
```
Grupp, Robert, et al. "Fast and automatic periacetabular osteotomy fragment pose estimation using intraoperatively implanted fiducials and single-view fluoroscopy." Physics in Medicine & Biology (2020).
----------------------------------------------------------------------
@article{grupp2020fast,
  title={Fast and automatic periacetabular osteotomy fragment pose estimation using intraoperatively implanted fiducials and single-view fluoroscopy},
  author={Grupp, Robert and Murphy, Ryan and Hegeman, Rachel and Alexander, Clayton and Unberath, Mathias and Otake, Yoshito and McArthur, Benjamin and Armand, Mehran and Taylor, Russell H},
  journal={Physics in Medicine \& Biology},
  year={2020},
  publisher={IOP Publishing}
}
```
```
Grupp, R., et al. "Pelvis surface estimation from partial CT for computer-aided pelvic osteotomies." Orthopaedic Proceedings. Vol. 98. No. SUPP_5. The British Editorial Society of Bone & Joint Surgery, 2016.
----------------------------------------------------------------------
@inproceedings{grupp2016pelvis,
  title={Pelvis surface estimation from partial {CT} for computer-aided pelvic osteotomies},
  author={Grupp, R and Otake, Y and Murphy, R and Parvizi, J and Armand, M and Taylor, R},
  booktitle={Orthopaedic Proceedings},
  volume={98},
  number={SUPP\_5},
  pages={55--55},
  year={2016},
  organization={The British Editorial Society of Bone \& Joint Surgery}
}
```
```
Grupp, Robert B., et al. "Smooth extrapolation of unknown anatomy via statistical shape models." Medical Imaging 2015: Image-Guided Procedures, Robotic Interventions, and Modeling. Vol. 9415. International Society for Optics and Photonics, 2015.
----------------------------------------------------------------------
@inproceedings{grupp2015smooth,
  title={Smooth extrapolation of unknown anatomy via statistical shape models},
  author={Grupp, Robert B and Chiang, H and Otake, Yoshito and Murphy, Ryan J and Gordon, Chad R and Armand, Mehran and Taylor, Russell H},
  booktitle={Medical Imaging 2015: Image-Guided Procedures, Robotic Interventions, and Modeling},
  volume={9415},
  pages={941524},
  year={2015},
  organization={International Society for Optics and Photonics}
}
```
