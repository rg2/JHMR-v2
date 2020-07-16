# Volume Cropping
This tool takes as input a volume and an axis aligned rectangular region of interest (ROI) and outputs a cropped version of the input according to the ROI.

## Example
First, create the lymph node CT volume (`lymph.nii.gz`) as described in [Example 1 of the DICOM converter tool](../convert_resample_dicom).
Next, load the volume into 3D Slicer and define a region of interest around the pelvis and femur bone structures. This is illustrated in the following figures:
![Slicer Axial ROI](zz_readme/slicer_roi_axial.png)
![Slicer Coronal ROI](zz_readme/slicer_roi_coronal.png)
![Slicer Sagittal ROI](zz_readme/slicer_roi_sagittal.png)

Save the annotation file to disk - an example is included [here](zz_readme/R.acsv).
Run the following command to create a cropped volume, named pelvis.nii.gz:
```
jhmr-crop-vol lymph.nii.gz R.acsv pelvis.nii.gz
```
The cropped volume file size is about 42 MB, while the uncropped volume is about 224 MB.
The cropping may be verified by loading `pelvis.nii.gz` into 3D Slicer and is illustrated below:
![Slicer Axial Crop](zz_readme/slicer_crop_axial.png)
![Slicer Coronal Crop](zz_readme/slicer_crop_coronal.png)
![Slicer Sagittal Crop](zz_readme/slicer_crop_sagittal.png)

This cropped volume is also used in examples for the following tools:
* [Mesh creation](../../mesh/create_mesh)