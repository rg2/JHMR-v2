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



#include "jhmrHDF5.h"

#include "jhmrITKBasicImageUtils.h"

namespace
{

using namespace jhmr;

template <class tScalar>
H5::DataSet WriteSingleScalarH5Helper(const std::string& field_name,
                                      const tScalar& field_val,
                                      H5::CommonFG* h5)
{
  const auto data_type = LookupH5DataType<tScalar>();

  H5::DataSet data_set = h5->createDataSet(field_name, data_type, H5S_SCALAR);  

  data_set.write(&field_val, data_type);

  return data_set;
}

template <class T>
H5::DataSet CreateVectorH5Helper(const std::string& field_name,
                                 const unsigned long len,
                                 H5::CommonFG* h5,
                                 const bool compress)
{
  using Scalar = typename std::conditional<std::is_same<bool,T>::value,unsigned char,T>::type;
  
  H5::DSetCreatPropList props;
  props.copy(H5::DSetCreatPropList::DEFAULT);
  
  if (compress)
  {
    // Maximum ~1 MB chunk size (fits in default chunk cache)
    const hsize_t chunk_dims = std::min(len, size_type((1024 * 1024 * 1) / sizeof(Scalar)));
    props.setChunk(1, &chunk_dims);
    props.setDeflate(9);
  }
  
  const hsize_t len_tmp = len;
  H5::DataSpace data_space(1, &len_tmp);

  const auto data_type = LookupH5DataType<Scalar>();

  return h5->createDataSet(field_name, data_type, data_space, props);
}

template <class T, class A>
H5::DataSet WriteVectorH5Helper(const std::string& field_name,
                                const std::vector<T,A>& v,
                                H5::CommonFG* h5,
                                const bool compress)
{
  using Scalar = T;
  
  H5::DataSet data_set = CreateVectorH5Helper<Scalar>(field_name, v.size(), h5, compress);
  data_set.write(&v[0], LookupH5DataType<Scalar>());

  return data_set; 
}

template <class T>
void WriteVectorElemH5Helper(const T& x, const unsigned long i, H5::DataSet* h5)
{
  H5::DataSpace f_ds = h5->getSpace();
  
  const hsize_t cur_idx = i;

  f_ds.selectElements(H5S_SELECT_SET, 1, &cur_idx);

  const hsize_t single_elem = 1;
  H5::DataSpace m_ds(1, &single_elem);

  h5->write(&x, LookupH5DataType<T>(), m_ds, f_ds);
}

template <class tScalar>
H5::DataSet CreateMatrixH5Helper(const std::string& field_name,
                                 const unsigned long num_rows,
                                 const unsigned long num_cols,
                                 H5::CommonFG* h5,
                                 const bool compress)
{
  using Scalar = tScalar;

  H5::DSetCreatPropList props;
  props.copy(H5::DSetCreatPropList::DEFAULT);
  
  if (compress)
  {
    // Maximum ~1 MB chunk size (fits in default chunk cache)
    
    constexpr hsize_t max_elems_per_dim = 1024 / sizeof(Scalar);
    const std::array<hsize_t,2> chunk_dims = { std::min(static_cast<hsize_t>(num_rows), max_elems_per_dim),
                                               std::min(static_cast<hsize_t>(num_cols), max_elems_per_dim) };

    props.setChunk(2, chunk_dims.data());
    props.setDeflate(9);
  }
  
  const std::array<hsize_t,2> mat_dims = { static_cast<hsize_t>(num_rows),
                                           static_cast<hsize_t>(num_cols) };
  H5::DataSpace data_space(2, mat_dims.data());

  const auto data_type = LookupH5DataType<Scalar>();

  return h5->createDataSet(field_name, data_type, data_space, props);
}

template <class tScalar, int tRows, int tCols, int tOpts, int tMaxRows, int tMaxCols>
H5::DataSet WriteMatrixH5Helper(const std::string& field_name,
                                const Eigen::Matrix<tScalar,tRows,tCols,tOpts,tMaxRows,tMaxCols>& mat,
                                H5::CommonFG* h5,
                                const bool compress)
{
  using Scalar = tScalar;

  H5::DataSet data_set = CreateMatrixH5Helper<Scalar>(field_name, mat.rows(), mat.cols(), h5, compress);

  const auto data_type = LookupH5DataType<Scalar>();

  // HDF5 is row-major

  if (tOpts & Eigen::RowMajor)
  {
    // input matrix is row-major, we can just write the entire memory block
    data_set.write(&mat(0,0), data_type);
  }
  else
  {
    // input matrix is column-major, we need to write a column at a time

    H5::DataSpace data_space = data_set.getSpace();

    const size_type nr = static_cast<size_type>(mat.rows());
    const size_type nc = static_cast<size_type>(mat.cols());
 
    std::array<hsize_t,2> file_start = { 0, 0 };

    const std::array<hsize_t,2> file_count = { nr, 1 };

    // data space for a single column
    const H5::DataSpace mem_col_data_space(2, file_count.data());

    for (size_type c = 0; c < nc; ++c)
    {
      // select the current column in the hdf5 file
      file_start[1] = c;
      data_space.selectHyperslab(H5S_SELECT_SET, file_count.data(), file_start.data());
      
      data_set.write(&mat(0,c), data_type, mem_col_data_space, data_space);
    }
  }

  return data_set; 
}

template <class tScalar>
void WriteMatrixRowH5Helper(const tScalar* row_buf, const unsigned long row_idx, H5::DataSet* h5)
{
  using Scalar = tScalar;

  H5::DataSpace ds_f = h5->getSpace();
  jhmrASSERT(ds_f.getSimpleExtentNdims() == 2);

  std::array<hsize_t,2> f_dims;
  ds_f.getSimpleExtentDims(f_dims.data());
  jhmrASSERT(row_idx < f_dims[0]);

  const std::array<hsize_t,2> m_dims = { 1, f_dims[1] };
  H5::DataSpace ds_m(2, m_dims.data());
  
  const std::array<hsize_t,2> f_start = { row_idx, 0 };

  ds_f.selectHyperslab(H5S_SELECT_SET, m_dims.data(), f_start.data());

  h5->write(row_buf, LookupH5DataType<Scalar>(), ds_m, ds_f);
}

template <class tScalar, unsigned int tN>
void WriteNDImageH5Helper(const itk::Image<tScalar,tN>* img, 
                          H5::CommonFG* h5,
                          const bool compress)
{
  using PixelScalar = tScalar;

  constexpr unsigned int kDIM = tN;

  // Set an attribute that indicates this is an N-D image
  SetStringAttr("jhmr-type", fmt::format("image-{}D", kDIM), h5);

  // first write the image metadata

  WriteMatrixH5("dir-mat", GetITKDirectionMatrix(img), h5, false);

  WriteMatrixH5("origin", GetITKOriginPoint(img), h5, false);

  const auto itk_spacing = img->GetSpacing();
  Eigen::Matrix<CoordScalar,kDIM,1> spacing;
  for (unsigned int i = 0; i < kDIM; ++i)
  {
    spacing[i] = itk_spacing[i];
  }
  WriteMatrixH5("spacing", spacing, h5, false);

  // Now write the pixel data

  const auto itk_size = img->GetLargestPossibleRegion().GetSize();

  std::array<hsize_t,kDIM> img_dims;
  for (unsigned int i = 0; i < kDIM; ++i)
  {
    // reverse the order - ITK buffer is "row major" but the index and
    // size orderings are reversed.
    img_dims[i] = itk_size[kDIM - 1 - i];
  }
  
  H5::DSetCreatPropList props;
  props.copy(H5::DSetCreatPropList::DEFAULT);

  if (compress)
  {
    std::array<hsize_t,kDIM> chunk_dims = img_dims;

    // for now 1 MB max chunk size
    constexpr unsigned long max_num_elems_for_chunk = (1 * 1024 * 1024)
                                                          / sizeof(PixelScalar);

    bool chunk_ok = false;

    while (!chunk_ok)
    {
      bool found_non_one_chunk_size = false;
      unsigned long elems_per_chunk = 1;
      for (const auto& cd : chunk_dims)
      {
        elems_per_chunk *= cd;
        
        if (cd != 1)
        {
          found_non_one_chunk_size = true;
        }
      }

      if (!found_non_one_chunk_size || (elems_per_chunk <= max_num_elems_for_chunk))
      {
        chunk_ok = true;
      }
      else
      {
        for (unsigned int cur_dim = 0; cur_dim < kDIM; ++cur_dim)
        {
          if (chunk_dims[cur_dim] != 1)
          {
            --chunk_dims[cur_dim];
            break;
          }
        }
      }
    }
    
    props.setChunk(kDIM, chunk_dims.data());
    props.setDeflate(9);
  }
  
  const auto data_type = LookupH5DataType<PixelScalar>();
  
  H5::DataSpace data_space(kDIM, img_dims.data());

  H5::DataSet data_set = h5->createDataSet("pixels", data_type, data_space, props);

  data_set.write(img->GetBufferPointer(), data_type);
}

template <class tMapIt>
void WriteLandmarksMapH5Helper(tMapIt map_begin, tMapIt map_end, H5::CommonFG* h5)
{
  using MapIt = tMapIt;

  SetStringAttr("jhmr-type", "lands-map", h5);

  for (MapIt it = map_begin; it != map_end; ++it)
  {
    WriteMatrixH5(it->first, it->second, h5, false);
  }
}

template <class tKey, class tVal>
void WriteLandmarksMapH5Helper(const std::unordered_map<tKey,tVal>& m, H5::CommonFG* h5)
{
  WriteLandmarksMapH5Helper(m.begin(), m.end(), h5);
}

template <class tScalar, int tRows, int tCols, int tOpts, int tMaxRows, int tMaxCols, class A>
H5::DataSet WriteListOfPointsAsMatrixH5Helper(const std::string& field_name,
                          const std::vector<Eigen::Matrix<tScalar,tRows,tCols,tOpts,tMaxRows,tMaxCols>,A>& pts,
                          H5::CommonFG* h5,
                          const bool compress)
{
  using Pt  = Eigen::Matrix<tScalar,tRows,tCols,tOpts,tMaxRows,tMaxCols>;
  using Mat = Eigen::Matrix<tScalar,Eigen::Dynamic,Eigen::Dynamic>;

  jhmrASSERT(!pts.empty());

  const int nr = pts[0].rows();
  const int nc = pts[0].cols();

  jhmrASSERT(nc == 1);
  
  const int num_pts = pts.size();

  Mat mat(nr,num_pts);

  for (int i = 0; i < num_pts; ++i)
  {
    mat.block(0,i,nr,1) = pts[i];
  }
 
  return WriteMatrixH5(field_name, mat, h5, compress);
}

template <class tScalar, unsigned long tDim>
H5::DataSet WriteListOfArraysAsMatrixH5Helper(const std::string& field_name,
                                              const std::vector<std::array<tScalar,tDim>>& arrays,
                                              H5::CommonFG* h5,
                                              const bool compress)
{
  constexpr int kDIM = tDim;

  using Mat = Eigen::Matrix<tScalar,Eigen::Dynamic,Eigen::Dynamic>;

  jhmrASSERT(!arrays.empty());

  const int num_arrays = static_cast<int>(arrays.size());
  
  Mat mat(kDIM, num_arrays);

  for (int array_idx = 0; array_idx < num_arrays; ++array_idx)
  {
    const auto& cur_array = arrays[array_idx];

    for (int r = 0; r < kDIM; ++r)
    {
      mat(r,array_idx) = cur_array[r];
    }
  }
  
  return WriteMatrixH5Helper(field_name, mat, h5, compress);
}

template <class tLabelScalar, unsigned int tN>
void WriteSegNDImageH5Helper(const itk::Image<tLabelScalar,tN>* img, 
                             H5::CommonFG* h5,
                             const std::unordered_map<tLabelScalar,std::string>& seg_labels_def,
                             const bool compress)
{
  using LabelScalar = tLabelScalar;

  constexpr unsigned int kDIM = tN;

  // Set an attribute that indicates this is an N-D image segmentation
  SetStringAttr("jhmr-type", fmt::format("image-seg-{}D", kDIM), h5);

  // write the label image
  {
    H5::Group seg_img_g = h5->createGroup("image");
    WriteImageH5(img, &seg_img_g, compress);
  }
  
  // write the labels def if available
  if (!seg_labels_def.empty())
  {
    H5::Group seg_labs_def_g = h5->createGroup("labels-def");
    
    for (const auto& kv : seg_labels_def)
    {
      WriteStringH5(fmt::format("{}", kv.first), kv.second, &seg_labs_def_g);
    }
  }
}

template <class tKey, class tVal>
void WriteMapAsArraysHelper(const std::unordered_map<tKey,tVal>& m, H5::CommonFG* h5,
                            const std::string& keys_g_name = "keys",
                            const std::string& vals_g_name = "vals",
                            const bool compress = true)
{
  using MapKey  = tKey;
  using KeyList = std::vector<MapKey>;
  using MapVal  = tVal;
  using ValList = std::vector<MapVal>;
  
  SetStringAttr("jhmr-type", "map-as-arrays", h5);
  SetStringAttr("jhmr-keys-g-name", keys_g_name, h5);
  SetStringAttr("jhmr-vals-g-name", vals_g_name, h5);

  const auto len = m.size();
  
  KeyList keys;
  ValList vals;

  keys.reserve(len);
  vals.reserve(len);
  
  for (const auto& kv : m)
  {
    keys.push_back(kv.first);
    vals.push_back(kv.second);
  }

  WriteVectorH5(keys_g_name, keys, h5, compress);
  WriteVectorH5(vals_g_name, vals, h5, compress);
}

template <class tScalar>
tScalar ReadSingleScalarH5Helper(const std::string& field_name,
                                 const H5::CommonFG& h5)
{
  using Scalar = tScalar;

  const H5::DataSet data_set = h5.openDataSet(field_name);
  
  Scalar x;
  data_set.read(&x, LookupH5DataType<Scalar>());

  return x;
}

template <class T>
std::vector<T> ReadVectorH5Helper(const std::string& field_name,
                                  const H5::CommonFG& h5)
{
  using Scalar = T;
  using Vec    = std::vector<Scalar>;

  const H5::DataSet data_set = h5.openDataSet(field_name);

  const H5::DataSpace data_space = data_set.getSpace();

  jhmrASSERT(data_space.getSimpleExtentNdims() == 1);

  hsize_t len = 0;
  data_space.getSimpleExtentDims(&len);

  Vec v(len);
  data_set.read(&v[0], LookupH5DataType<Scalar>());

  return v;
}

template <class tScalar>
Eigen::Matrix<tScalar,Eigen::Dynamic,Eigen::Dynamic,Eigen::RowMajor|Eigen::AutoAlign>
ReadMatrixH5Helper(const std::string& field_name, const H5::CommonFG& h5)
{
  using Scalar = tScalar;
  using Mat    = Eigen::Matrix<Scalar,Eigen::Dynamic,Eigen::Dynamic,
                               Eigen::RowMajor|Eigen::AutoAlign>;

  const H5::DataSet data_set = h5.openDataSet(field_name);

  const H5::DataSpace data_space = data_set.getSpace();

  jhmrASSERT(data_space.getSimpleExtentNdims() == 2);

  std::array<hsize_t,2> dims = { 0, 0 };
  data_space.getSimpleExtentDims(dims.data());

  static_assert(!std::is_same<Scalar,bool>::value,
                "read bool matrix not implemented! need to read unsigned char!");

  Mat m(dims[0], dims[1]);
  data_set.read(&m(0,0), LookupH5DataType<Scalar>());

  return m;
}

template <class tScalar, unsigned int tN>
typename itk::Image<tScalar,tN>::Pointer
ReadNDImageH5Helper(const H5::CommonFG& h5)
{
  using PixelScalar = tScalar;

  constexpr unsigned int kDIM = tN;

  using Img    = itk::Image<PixelScalar,kDIM>;
  using ImgPtr = typename Img::Pointer;

  using CoordScalar = typename Img::SpacingValueType;

  const auto dir_mat   = ReadMatrixH5Float("dir-mat", h5);
  const auto origin_pt = ReadMatrixH5Float("origin",  h5);
  const auto spacing   = ReadMatrixH5Float("spacing", h5);
  
  const H5::DataSet data_set = h5.openDataSet("pixels");

  const H5::DataSpace data_space = data_set.getSpace();

  jhmrASSERT(data_space.getSimpleExtentNdims() == kDIM);

  std::array<hsize_t,kDIM> dims;
  data_space.getSimpleExtentDims(dims.data());

  ImgPtr img = Img::New();

  SetITKDirectionMatrix(img.GetPointer(), dir_mat);
  SetITKOriginPoint(img.GetPointer(), origin_pt);

  typename Img::SpacingType itk_spacing;
  for (unsigned int i = 0; i < kDIM; ++i)
  {
    itk_spacing[i] = spacing(i);
  }
  img->SetSpacing(itk_spacing);

  typename Img::RegionType reg;
  for (unsigned int i = 0; i < kDIM; ++i)
  {
    reg.SetIndex(i,0);
    reg.SetSize(i, dims[kDIM - 1 - i]);  // See WriteITKImageH5
  }
  img->SetRegions(reg);

  img->Allocate();

  data_set.read(img->GetBufferPointer(), LookupH5DataType<PixelScalar>());

  return img;
}

template <class tPt>
std::unordered_map<std::string,tPt>
ReadLandmarksMapH5Helper(const H5::CommonFG& h5)
{
  using Pt = tPt;
  using LandsMap = std::unordered_map<std::string,Pt>;
  using Scalar = typename Pt::Scalar;

  const hsize_t num_lands = h5.getNumObjs();

  LandsMap lands;

  if (num_lands)
  {
    lands.reserve(num_lands);

    for (hsize_t l = 0; l < num_lands; ++l)
    {
      const std::string land_name = h5.getObjnameByIdx(l);

      lands.insert(typename LandsMap::value_type(land_name,
                        ReadMatrixH5Helper<Scalar>(land_name, h5)));
    }
  }

  return lands;
}

template <class tPt>
std::vector<tPt> ReadLandsAsPtCloudH5Helper(const H5::CommonFG& h5)
{
  using Pt     = tPt;
  using PtList = std::vector<Pt>;
  using Scalar = typename Pt::Scalar;

  const hsize_t num_lands = h5.getNumObjs();

  PtList pts;

  if (num_lands)
  {
    pts.reserve(num_lands);

    for (hsize_t l = 0; l < num_lands; ++l)
    {
      pts.push_back(ReadMatrixH5Helper<Scalar>(h5.getObjnameByIdx(l), h5));
    }
  }

  return pts;
}

template <class tScalar, int tDim = Eigen::Dynamic>
std::vector<Eigen::Matrix<tScalar,tDim,1>>
ReadListOfPointsFromMatrixH5Helper(const std::string& field_name, const H5::CommonFG& h5)
{
  using Scalar = tScalar;

  constexpr int kDIM = tDim;

  using Pt = Eigen::Matrix<Scalar,kDIM,1>;
  
  using PtList = std::vector<Pt>;

  const auto mat = ReadMatrixH5Helper<Scalar>(field_name, h5);
  
  const int dim = mat.rows();

  jhmrASSERT((kDIM == Eigen::Dynamic) || (kDIM == dim));

  const int num_pts = mat.cols();

  PtList pts;
  pts.reserve(num_pts);

  for (int i = 0; i < num_pts; ++i)
  {
    pts.push_back(mat.block(0,i,dim,1));
  }

  return pts;
}

template <class tScalar, unsigned long tDim>
std::vector<std::array<tScalar,tDim>>
ReadListOfArraysFromMatrixH5Helper(const std::string& field_name, const H5::CommonFG& h5)
{
  using Scalar = tScalar;

  constexpr unsigned long kDIM = tDim;

  using Array        = std::array<Scalar,kDIM>;
  using ListOfArrays = std::vector<Array>;

  const auto mat = ReadMatrixH5Helper<Scalar>(field_name, h5);

  jhmrASSERT(static_cast<unsigned long>(mat.rows() == kDIM));

  const int num_arrays = mat.cols();

  ListOfArrays arrays(num_arrays);
  
  for (int array_idx = 0; array_idx < num_arrays; ++array_idx)
  {
    auto& cur_array = arrays[array_idx];

    for (unsigned long i = 0; i < kDIM; ++i)
    {
      cur_array[i] = mat(i,array_idx);
    }
  }

  return arrays;
}

}  // un-named

H5::DataType jhmr::GetH5StringDataType()
{
  return LookupH5DataType<std::string>();
}

H5::DataType jhmr::GetH5StringDataType(const std::string& s)
{
  return H5::StrType(H5::PredType::C_S1, s.size());
}

bool jhmr::SetStringAttr(const std::string& key, const std::string& val, H5::CommonFG* h5)
{
  bool attr_set = false;

  H5::Group* h5_g = dynamic_cast<H5::Group*>(h5);

  if (h5_g)
  {
    const auto dt = GetH5StringDataType(val);

    H5::Attribute attr = h5_g->createAttribute(key, dt, H5S_SCALAR);

    attr.write(dt, val);

    attr_set = true;
  }
  
  return attr_set;
}

H5::DataSet jhmr::WriteStringH5(const std::string& field_name,
                                const std::string& field_val,
                                H5::CommonFG* h5,
                                const bool compress)
{
  H5::DSetCreatPropList props;
  props.copy(H5::DSetCreatPropList::DEFAULT);
  
  if (compress)
  {
    // NOTE: THIS DOES NOT SEEM TO BE CORRECT - CRASHES ABOUT MISMATCH
    //       OF DIMENSION
    const hsize_t chunk_dims = std::min(field_val.size(),
                                        std::string::size_type(1024));
    props.setChunk(1, &chunk_dims);
    props.setDeflate(9);
  }

  const auto data_type = GetH5StringDataType(field_val);

  H5::DataSet data_set = h5->createDataSet(field_name, data_type,
                                           H5S_SCALAR, props);

  data_set.write(field_val, data_type);

  return data_set;
}

H5::DataSet jhmr::WriteSingleScalarH5(const std::string& field_name,
                                      const unsigned char& field_val,
                                      H5::CommonFG* h5)
{
  return WriteSingleScalarH5Helper<unsigned char>(field_name, field_val, h5);
}

H5::DataSet jhmr::WriteSingleScalarH5(const std::string& field_name,
                                      const char& field_val,
                                      H5::CommonFG* h5)
{
  return WriteSingleScalarH5Helper<char>(field_name, field_val, h5);
}

H5::DataSet jhmr::WriteSingleScalarH5(const std::string& field_name,
                                      const unsigned short& field_val,
                                      H5::CommonFG* h5)
{
  return WriteSingleScalarH5Helper<unsigned short>(field_name, field_val, h5);
}

H5::DataSet jhmr::WriteSingleScalarH5(const std::string& field_name,
                                      const short& field_val,
                                      H5::CommonFG* h5)
{
  return WriteSingleScalarH5Helper<short>(field_name, field_val, h5);
}

H5::DataSet jhmr::WriteSingleScalarH5(const std::string& field_name,
                                      const unsigned int& field_val,
                                      H5::CommonFG* h5)
{
  return WriteSingleScalarH5Helper<unsigned int>(field_name, field_val, h5);
}

H5::DataSet jhmr::WriteSingleScalarH5(const std::string& field_name,
                                      const int& field_val,
                                      H5::CommonFG* h5)
{
  return WriteSingleScalarH5Helper<int>(field_name, field_val, h5);
}

H5::DataSet jhmr::WriteSingleScalarH5(const std::string& field_name,
                                      const unsigned long& field_val,
                                      H5::CommonFG* h5)
{
  return WriteSingleScalarH5Helper<unsigned long>(field_name, field_val, h5);
}

H5::DataSet jhmr::WriteSingleScalarH5(const std::string& field_name,
                                      const long& field_val,
                                      H5::CommonFG* h5)
{
  return WriteSingleScalarH5Helper<long>(field_name, field_val, h5);
}

H5::DataSet jhmr::WriteSingleScalarH5(const std::string& field_name,
                                      const float& field_val,
                                      H5::CommonFG* h5)
{
  return WriteSingleScalarH5Helper<float>(field_name, field_val, h5);
}

H5::DataSet jhmr::WriteSingleScalarH5(const std::string& field_name,
                                      const double& field_val,
                                      H5::CommonFG* h5)
{
  return WriteSingleScalarH5Helper<double>(field_name, field_val, h5);
}

H5::DataSet jhmr::WriteSingleScalarH5(const std::string& field_name,
                                      const bool& field_val,
                                      H5::CommonFG* h5)
{
  return WriteSingleScalarH5Helper<unsigned char>(field_name, static_cast<unsigned char>(field_val), h5);
}

H5::DataSet jhmr::CreateVectorH5UChar(const std::string& field_name,
                                      const unsigned long len,
                                      H5::CommonFG* h5,
                                      const bool compress)
{
  return CreateVectorH5Helper<unsigned char>(field_name, len, h5, compress);
}

H5::DataSet jhmr::CreateVectorH5Char(const std::string& field_name,
                                     const unsigned long len,
                                     H5::CommonFG* h5,
                                     const bool compress)
{
  return CreateVectorH5Helper<char>(field_name, len, h5, compress);
}

H5::DataSet jhmr::CreateVectorH5UShort(const std::string& field_name,
                                       const unsigned long len,
                                       H5::CommonFG* h5,
                                       const bool compress)
{
  return CreateVectorH5Helper<unsigned short>(field_name, len, h5, compress);
}

H5::DataSet jhmr::CreateVectorH5Short(const std::string& field_name,
                                      const unsigned long len,
                                      H5::CommonFG* h5,
                                      const bool compress)
{
  return CreateVectorH5Helper<short>(field_name, len, h5, compress);
}

H5::DataSet jhmr::CreateVectorH5UInt(const std::string& field_name,
                                     const unsigned long len,
                                     H5::CommonFG* h5,
                                     const bool compress)
{
  return CreateVectorH5Helper<unsigned int>(field_name, len, h5, compress);
}

H5::DataSet jhmr::CreateVectorH5Int(const std::string& field_name,
                                    const unsigned long len,
                                    H5::CommonFG* h5,
                                    const bool compress)
{
  return CreateVectorH5Helper<int>(field_name, len, h5, compress);
}

H5::DataSet jhmr::CreateVectorH5ULong(const std::string& field_name,
                                      const unsigned long len,
                                      H5::CommonFG* h5,
                                      const bool compress)
{
  return CreateVectorH5Helper<unsigned long>(field_name, len, h5, compress);
}

H5::DataSet jhmr::CreateVectorH5Long(const std::string& field_name,
                                     const unsigned long len,
                                     H5::CommonFG* h5,
                                     const bool compress)
{
  return CreateVectorH5Helper<long>(field_name, len, h5, compress);
}

H5::DataSet jhmr::CreateVectorH5Float(const std::string& field_name,
                                      const unsigned long len,
                                      H5::CommonFG* h5,
                                      const bool compress)
{
  return CreateVectorH5Helper<float>(field_name, len, h5, compress);
}

H5::DataSet jhmr::CreateVectorH5Double(const std::string& field_name,
                                       const unsigned long len,
                                       H5::CommonFG* h5,
                                       const bool compress)
{
  return CreateVectorH5Helper<double>(field_name, len, h5, compress);
}

H5::DataSet jhmr::CreateVectorH5Bool(const std::string& field_name,
                                     const unsigned long len,
                                     H5::CommonFG* h5,
                                     const bool compress)
{
  return CreateVectorH5Helper<unsigned char>(field_name, len, h5, compress);
}

H5::DataSet jhmr::WriteVectorH5(const std::string& field_name,
                                const std::vector<unsigned char>& v,
                                H5::CommonFG* h5,
                                const bool compress)
{
  return WriteVectorH5Helper<unsigned char>(field_name, v, h5, compress);
}

H5::DataSet jhmr::WriteVectorH5(const std::string& field_name,
                                const std::vector<char>& v,
                                H5::CommonFG* h5,
                                const bool compress)
{
  return WriteVectorH5Helper<char>(field_name, v, h5, compress);
}

H5::DataSet jhmr::WriteVectorH5(const std::string& field_name,
                                const std::vector<unsigned short>& v,
                                H5::CommonFG* h5,
                                const bool compress)
{
  return WriteVectorH5Helper<unsigned short>(field_name, v, h5, compress);
}

H5::DataSet jhmr::WriteVectorH5(const std::string& field_name,
                                const std::vector<short>& v,
                                H5::CommonFG* h5,
                                const bool compress)
{
  return WriteVectorH5Helper<short>(field_name, v, h5, compress);
}

H5::DataSet jhmr::WriteVectorH5(const std::string& field_name,
                                const std::vector<unsigned int>& v,
                                H5::CommonFG* h5,
                                const bool compress)
{
  return WriteVectorH5Helper<unsigned int>(field_name, v, h5, compress);
}

H5::DataSet jhmr::WriteVectorH5(const std::string& field_name,
                                const std::vector<int>& v,
                                H5::CommonFG* h5,
                                const bool compress)
{
  return WriteVectorH5Helper<int>(field_name, v, h5, compress);
}

H5::DataSet jhmr::WriteVectorH5(const std::string& field_name,
                                const std::vector<unsigned long>& v,
                                H5::CommonFG* h5,
                                const bool compress)
{
  return WriteVectorH5Helper<unsigned long>(field_name, v, h5, compress);
}

H5::DataSet jhmr::WriteVectorH5(const std::string& field_name,
                                const std::vector<long>& v,
                                H5::CommonFG* h5,
                                const bool compress)
{
  return WriteVectorH5Helper<long>(field_name, v, h5, compress);
}

H5::DataSet jhmr::WriteVectorH5(const std::string& field_name,
                                const std::vector<float>& v,
                                H5::CommonFG* h5,
                                const bool compress)
{
  return WriteVectorH5Helper<float>(field_name, v, h5, compress);
}

H5::DataSet jhmr::WriteVectorH5(const std::string& field_name,
                                const std::vector<double>& v,
                                H5::CommonFG* h5,
                                const bool compress)
{
  return WriteVectorH5Helper<double>(field_name, v, h5, compress);
}

H5::DataSet jhmr::WriteVectorH5(const std::string& field_name,
                                const std::vector<bool>& v,
                                H5::CommonFG* h5,
                                const bool compress)
{
  std::vector<unsigned char> tmp;
  tmp.reserve(v.size());

  for (const auto& b : v)
  {
    tmp.push_back(b);
  }

  return WriteVectorH5(field_name, tmp, h5, compress);
}

void jhmr::WriteVectorElemH5(const unsigned char& x, const unsigned long i, H5::DataSet* h5)
{
  WriteVectorElemH5Helper<unsigned char>(x, i, h5);
}

void jhmr::WriteVectorElemH5(const char& x, const unsigned long i, H5::DataSet* h5)
{
  WriteVectorElemH5Helper<char>(x, i, h5);
}

void jhmr::WriteVectorElemH5(const unsigned short& x, const unsigned long i, H5::DataSet* h5)
{
  WriteVectorElemH5Helper<unsigned short>(x, i, h5);
}

void jhmr::WriteVectorElemH5(const short& x, const unsigned long i, H5::DataSet* h5)
{
  WriteVectorElemH5Helper<short>(x, i, h5);
}

void jhmr::WriteVectorElemH5(const unsigned int& x, const unsigned long i, H5::DataSet* h5)
{
  WriteVectorElemH5Helper<unsigned int>(x, i, h5);
}

void jhmr::WriteVectorElemH5(const int& x, const unsigned long i, H5::DataSet* h5)
{
  WriteVectorElemH5Helper<int>(x, i, h5);
}

void jhmr::WriteVectorElemH5(const unsigned long& x, const unsigned long i, H5::DataSet* h5)
{
  WriteVectorElemH5Helper<unsigned long>(x, i, h5);
}

void jhmr::WriteVectorElemH5(const long& x, const unsigned long i, H5::DataSet* h5)
{
  WriteVectorElemH5Helper<long>(x, i, h5);
}

void jhmr::WriteVectorElemH5(const float& x, const unsigned long i, H5::DataSet* h5)
{
  WriteVectorElemH5Helper<float>(x, i, h5);
}

void jhmr::WriteVectorElemH5(const double& x, const unsigned long i, H5::DataSet* h5)
{
  WriteVectorElemH5Helper<double>(x, i, h5);
}

void jhmr::WriteVectorElemH5(const bool& x, const unsigned long i, H5::DataSet* h5)
{
  WriteVectorElemH5(static_cast<unsigned char>(x), i, h5);
}

H5::DataSet jhmr::CreateMatrixH5Float(const std::string& field_name,
                                      const unsigned long num_rows,
                                      const unsigned long num_cols,
                                      H5::CommonFG* h5,
                                      const bool compress)
{
  return CreateMatrixH5Helper<float>(field_name, num_rows, num_cols, h5, compress);
}

H5::DataSet jhmr::CreateMatrixH5Double(const std::string& field_name,
                                       const unsigned long num_rows,
                                       const unsigned long num_cols,
                                       H5::CommonFG* h5,
                                       const bool compress)
{
  return CreateMatrixH5Helper<double>(field_name, num_rows, num_cols, h5, compress);
}

H5::DataSet jhmr::WriteMatrixH5(const std::string& field_name,
                                const Pt2& mat,
                                H5::CommonFG* h5,
                                const bool compress)
{
  return WriteMatrixH5Helper(field_name, mat, h5, compress);
}

H5::DataSet jhmr::WriteMatrixH5(const std::string& field_name,
                                const Pt3& mat,
                                H5::CommonFG* h5,
                                const bool compress)
{
  return WriteMatrixH5Helper(field_name, mat, h5, compress);
}

H5::DataSet jhmr::WriteMatrixH5(const std::string& field_name,
                                const Pt4& mat,
                                H5::CommonFG* h5,
                                const bool compress)
{
  return WriteMatrixH5Helper(field_name, mat, h5, compress);
}

H5::DataSet jhmr::WriteMatrixH5(const std::string& field_name,
                                const Pt5& mat,
                                H5::CommonFG* h5,
                                const bool compress)
{
  return WriteMatrixH5Helper(field_name, mat, h5, compress);
}

H5::DataSet jhmr::WriteMatrixH5(const std::string& field_name,
                                const Pt6& mat,
                                H5::CommonFG* h5,
                                const bool compress)
{
  return WriteMatrixH5Helper(field_name, mat, h5, compress);
}

H5::DataSet jhmr::WriteMatrixH5(const std::string& field_name,
                                const PtN& mat,
                                H5::CommonFG* h5,
                                const bool compress)
{
  return WriteMatrixH5Helper(field_name, mat, h5, compress);
}

H5::DataSet jhmr::WriteMatrixH5(const std::string& field_name,
                                const Mat2x2& mat,
                                H5::CommonFG* h5,
                                const bool compress)
{
  return WriteMatrixH5Helper(field_name, mat, h5, compress);
}

H5::DataSet jhmr::WriteMatrixH5(const std::string& field_name,
                                const Mat3x3& mat,
                                H5::CommonFG* h5,
                                const bool compress)
{
  return WriteMatrixH5Helper(field_name, mat, h5, compress);
}

H5::DataSet jhmr::WriteMatrixH5(const std::string& field_name,
                                const Mat4x4& mat,
                                H5::CommonFG* h5,
                                const bool compress)
{
  return WriteMatrixH5Helper(field_name, mat, h5, compress);
}

H5::DataSet jhmr::WriteMatrixH5(const std::string& field_name,
                                const Mat3x4& mat,
                                H5::CommonFG* h5,
                                const bool compress)
{
  return WriteMatrixH5Helper(field_name, mat, h5, compress);
}

H5::DataSet jhmr::WriteMatrixH5(const std::string& field_name,
                                const MatMxN& mat,
                                H5::CommonFG* h5,
                                const bool compress)
{
  return WriteMatrixH5Helper(field_name, mat, h5, compress);
}

void jhmr::WriteMatrixRowH5(const float* row_buf, const unsigned long row_idx, H5::DataSet* h5)
{
  WriteMatrixRowH5Helper(row_buf, row_idx, h5);
}

void jhmr::WriteMatrixRowH5(const double* row_buf, const unsigned long row_idx, H5::DataSet* h5)
{
  WriteMatrixRowH5Helper(row_buf, row_idx, h5);
}

H5::DataSet jhmr::WriteAffineTransform4x4(const std::string& field_name,
                                          const FrameTransform& xform,
                                          H5::CommonFG* h5,
                                          const bool compress = true)
{
  return WriteMatrixH5(field_name, xform.matrix(), h5, compress);
}

void jhmr::WriteImageH5(const itk::Image<unsigned char,2>* img, 
                        H5::CommonFG* h5,
                        const bool compress)
{
  WriteNDImageH5Helper(img, h5, compress);
}

void jhmr::WriteImageH5(const itk::Image<unsigned short,2>* img, 
                        H5::CommonFG* h5,
                        const bool compress)
{
  WriteNDImageH5Helper(img, h5, compress);
}

void jhmr::WriteImageH5(const itk::Image<short,2>* img, 
                        H5::CommonFG* h5,
                        const bool compress)
{
  WriteNDImageH5Helper(img, h5, compress);
}

void jhmr::WriteImageH5(const itk::Image<float,2>* img, 
                        H5::CommonFG* h5,
                        const bool compress)
{
  WriteNDImageH5Helper(img, h5, compress);
}

void jhmr::WriteImageH5(const itk::Image<double,2>* img, 
                        H5::CommonFG* h5,
                        const bool compress)
{
  WriteNDImageH5Helper(img, h5, compress);
}

void jhmr::WriteImageH5(const itk::Image<unsigned char,3>* img, 
                        H5::CommonFG* h5,
                        const bool compress)
{
  WriteNDImageH5Helper(img, h5, compress);
}

void jhmr::WriteImageH5(const itk::Image<unsigned short,3>* img, 
                        H5::CommonFG* h5,
                        const bool compress)
{
  WriteNDImageH5Helper(img, h5, compress);
}

void jhmr::WriteImageH5(const itk::Image<short,3>* img, 
                        H5::CommonFG* h5,
                        const bool compress)
{
  WriteNDImageH5Helper(img, h5, compress);
}

void jhmr::WriteImageH5(const itk::Image<float,3>* img, 
                        H5::CommonFG* h5,
                        const bool compress)
{
  WriteNDImageH5Helper(img, h5, compress);
}

void jhmr::WriteImageH5(const itk::Image<double,3>* img, 
                        H5::CommonFG* h5,
                        const bool compress)
{
  WriteNDImageH5Helper(img, h5, compress);
}

void jhmr::WriteLandmarksMapH5(const LandMap2& m, H5::CommonFG* h5)
{
  WriteLandmarksMapH5Helper(m, h5);
}

void jhmr::WriteLandmarksMapH5(const LandMap3& m, H5::CommonFG* h5)
{
  WriteLandmarksMapH5Helper(m, h5);
}

void jhmr::WriteLandmarksMapH5(const LandMap4& m, H5::CommonFG* h5)
{
  WriteLandmarksMapH5Helper(m, h5);
}

H5::DataSet jhmr::WriteListOfPointsAsMatrixH5(const std::string& field_name,
                                              const Pt2List& pts,
                                              H5::CommonFG* h5,
                                              const bool compress)
{
  return WriteListOfPointsAsMatrixH5Helper(field_name, pts, h5, compress);
}

H5::DataSet jhmr::WriteListOfPointsAsMatrixH5(const std::string& field_name,
                                              const Pt3List& pts,
                                              H5::CommonFG* h5,
                                              const bool compress)
{
  return WriteListOfPointsAsMatrixH5Helper(field_name, pts, h5, compress);
}

H5::DataSet jhmr::WriteListOfArraysToMatrixH5(const std::string& field_name,
                                              const std::vector<std::array<size_type,3>>& arrays,
                                              H5::CommonFG* h5,
                                              const bool compress)
{
  return WriteListOfArraysAsMatrixH5Helper(field_name, arrays, h5, compress);
}

void jhmr::WriteSegImageH5(const itk::Image<unsigned char,2>* img, 
                           H5::CommonFG* h5,
                           const std::unordered_map<unsigned char,std::string>& seg_labels_def,
                           const bool compress)
{
  WriteSegNDImageH5Helper(img, h5, seg_labels_def, compress);
}

void jhmr::WriteSegImageH5(const itk::Image<unsigned short,2>* img, 
                           H5::CommonFG* h5,
                           const std::unordered_map<unsigned short,std::string>& seg_labels_def,
                           const bool compress)
{
  WriteSegNDImageH5Helper(img, h5, seg_labels_def, compress);
}

void jhmr::WriteSegImageH5(const itk::Image<unsigned char,3>* img, 
                           H5::CommonFG* h5,
                           const std::unordered_map<unsigned char,std::string>& seg_labels_def,
                           const bool compress)
{
  WriteSegNDImageH5Helper(img, h5, seg_labels_def, compress);
}

void jhmr::WriteSegImageH5(const itk::Image<unsigned short,3>* img, 
                           H5::CommonFG* h5,
                           const std::unordered_map<unsigned short,std::string>& seg_labels_def,
                           const bool compress)
{
  WriteSegNDImageH5Helper(img, h5, seg_labels_def, compress);
}

unsigned char jhmr::ReadSingleScalarH5UChar(const std::string& field_name,
                                            const H5::CommonFG& h5)
{
  return ReadSingleScalarH5Helper<unsigned char>(field_name, h5);
}

char jhmr::ReadSingleScalarH5Char(const std::string& field_name,
                                  const H5::CommonFG& h5)
{
  return ReadSingleScalarH5Helper<char>(field_name, h5);
}

unsigned short jhmr::ReadSingleScalarH5UShort(const std::string& field_name,
                                              const H5::CommonFG& h5)
{
  return ReadSingleScalarH5Helper<unsigned short>(field_name, h5);
}

short jhmr::ReadSingleScalarH5Short(const std::string& field_name,
                                    const H5::CommonFG& h5)
{
  return ReadSingleScalarH5Helper<short>(field_name, h5);
}

unsigned int jhmr::ReadSingleScalarH5UInt(const std::string& field_name,
                                          const H5::CommonFG& h5)
{
  return ReadSingleScalarH5Helper<unsigned int>(field_name, h5);
}

int jhmr::ReadSingleScalarH5Int(const std::string& field_name,
                                const H5::CommonFG& h5)
{
  return ReadSingleScalarH5Helper<int>(field_name, h5);
}

unsigned long jhmr::ReadSingleScalarH5ULong(const std::string& field_name,
                                            const H5::CommonFG& h5)
{
  return ReadSingleScalarH5Helper<unsigned long>(field_name, h5);
}

long jhmr::ReadSingleScalarH5Long(const std::string& field_name,
                                  const H5::CommonFG& h5)
{
  return ReadSingleScalarH5Helper<long>(field_name, h5);
}

float jhmr::ReadSingleScalarH5Float(const std::string& field_name,
                                    const H5::CommonFG& h5)
{
  return ReadSingleScalarH5Helper<float>(field_name, h5);
}

double jhmr::ReadSingleScalarH5Double(const std::string& field_name,
                                      const H5::CommonFG& h5)
{
  return ReadSingleScalarH5Helper<double>(field_name, h5);
}

bool jhmr::ReadSingleScalarH5Bool(const std::string& field_name,
                                  const H5::CommonFG& h5)
{
  return static_cast<bool>(ReadSingleScalarH5UChar(field_name, h5));
}

std::vector<unsigned char>
jhmr::ReadVectorH5UChar(const std::string& field_name, const H5::CommonFG& h5)
{
  return ReadVectorH5Helper<unsigned char>(field_name, h5);
}

std::vector<char>
jhmr::ReadVectorH5Char(const std::string& field_name, const H5::CommonFG& h5)
{
  return ReadVectorH5Helper<char>(field_name, h5);
}

std::vector<unsigned short>
jhmr::ReadVectorH5UShort(const std::string& field_name, const H5::CommonFG& h5)
{
  return ReadVectorH5Helper<unsigned short>(field_name, h5);
}

std::vector<short>
jhmr::ReadVectorH5Short(const std::string& field_name, const H5::CommonFG& h5)
{
  return ReadVectorH5Helper<short>(field_name, h5);
}

std::vector<unsigned int>
jhmr::ReadVectorH5UInt(const std::string& field_name, const H5::CommonFG& h5)
{
  return ReadVectorH5Helper<unsigned int>(field_name, h5);
}

std::vector<int>
jhmr::ReadVectorH5Int(const std::string& field_name, const H5::CommonFG& h5)
{
  return ReadVectorH5Helper<int>(field_name, h5);
}

std::vector<unsigned long>
jhmr::ReadVectorH5ULong(const std::string& field_name, const H5::CommonFG& h5)
{
  return ReadVectorH5Helper<unsigned long>(field_name, h5);
}

std::vector<long>
jhmr::ReadVectorH5Long(const std::string& field_name, const H5::CommonFG& h5)
{
  return ReadVectorH5Helper<long>(field_name, h5);
}

std::vector<float>
jhmr::ReadVectorH5Float(const std::string& field_name, const H5::CommonFG& h5)
{
  return ReadVectorH5Helper<float>(field_name, h5);
}

std::vector<double>
jhmr::ReadVectorH5Double(const std::string& field_name, const H5::CommonFG& h5)
{
  return ReadVectorH5Helper<double>(field_name, h5);
}

std::vector<bool>
jhmr::ReadVectorH5Bool(const std::string& field_name, const H5::CommonFG& h5)
{
  const auto hbool_vec = ReadVectorH5UChar(field_name, h5);

  std::vector<bool> v;
  v.reserve(hbool_vec.size());

  for (const auto& b : hbool_vec)
  {
    v.push_back(b);
  }

  return v;
}

jhmr::MatMxN jhmr::ReadMatrixH5Float(const std::string& field_name, const H5::CommonFG& h5)
{
  return ReadMatrixH5Helper<float>(field_name, h5);
}

jhmr::MatMxN_d jhmr::ReadMatrixH5Double(const std::string& field_name, const H5::CommonFG& h5)
{
  return ReadMatrixH5Helper<double>(field_name, h5);
}

jhmr::FrameTransform
jhmr::ReadAffineTransform4x4H5(const std::string& field_name,
                               const H5::CommonFG& h5)
{
  FrameTransform xform;

  xform.matrix() = ReadMatrixH5Helper<CoordScalar>(field_name, h5);

  return xform;
}

itk::Image<unsigned char,2>::Pointer
jhmr::ReadITKImageH5UChar2D(const H5::CommonFG& h5)
{
  return ReadNDImageH5Helper<unsigned char,2>(h5);
}

itk::Image<char,2>::Pointer
jhmr::ReadITKImageH5Char2D(const H5::CommonFG& h5)
{
  return ReadNDImageH5Helper<char,2>(h5);
}

itk::Image<unsigned short,2>::Pointer
jhmr::ReadITKImageH5UShort2D(const H5::CommonFG& h5)
{
  return ReadNDImageH5Helper<unsigned short,2>(h5);
}

itk::Image<short,2>::Pointer
jhmr::ReadITKImageH5Short2D(const H5::CommonFG& h5)
{
  return ReadNDImageH5Helper<short,2>(h5);
}

itk::Image<float,2>::Pointer
jhmr::ReadITKImageH5Float2D(const H5::CommonFG& h5)
{
  return ReadNDImageH5Helper<float,2>(h5);
}

itk::Image<double,2>::Pointer
jhmr::ReadITKImageH5Double2D(const H5::CommonFG& h5)
{
  return ReadNDImageH5Helper<double,2>(h5);
}

itk::Image<unsigned char,3>::Pointer
jhmr::ReadITKImageH5UChar3D(const H5::CommonFG& h5)
{
  return ReadNDImageH5Helper<unsigned char,3>(h5);
}

itk::Image<char,3>::Pointer
jhmr::ReadITKImageH5Char3D(const H5::CommonFG& h5)
{
  return ReadNDImageH5Helper<char,3>(h5);
}

itk::Image<unsigned short,3>::Pointer
jhmr::ReadITKImageH5UShort3D(const H5::CommonFG& h5)
{
  return ReadNDImageH5Helper<unsigned short,3>(h5);
}

itk::Image<short,3>::Pointer
jhmr::ReadITKImageH5Short3D(const H5::CommonFG& h5)
{
  return ReadNDImageH5Helper<short,3>(h5);
}

itk::Image<float,3>::Pointer
jhmr::ReadITKImageH5Float3D(const H5::CommonFG& h5)
{
  return ReadNDImageH5Helper<float,3>(h5);
}

itk::Image<double,3>::Pointer
jhmr::ReadITKImageH5Double3D(const H5::CommonFG& h5)
{
  return ReadNDImageH5Helper<double,3>(h5);
}

jhmr::LandMap2 jhmr::ReadLandmarksMapH5Pt2(const H5::CommonFG& h5)
{
  return ReadLandmarksMapH5Helper<Pt2>(h5);
}

jhmr::LandMap3 jhmr::ReadLandmarksMapH5Pt3(const H5::CommonFG& h5)
{
  return ReadLandmarksMapH5Helper<Pt3>(h5);
}

jhmr::LandMap4 jhmr::ReadLandmarksMapH5Pt4(const H5::CommonFG& h5)
{
  return ReadLandmarksMapH5Helper<Pt4>(h5);
}

jhmr::Pt2List jhmr::ReadLandsAsPtCloudH5Pt2(const H5::CommonFG& h5)
{
  return ReadLandsAsPtCloudH5Helper<Pt2>(h5);
}

jhmr::Pt3List jhmr::ReadLandsAsPtCloudH5Pt3(const H5::CommonFG& h5)
{
  return ReadLandsAsPtCloudH5Helper<Pt3>(h5);
}

jhmr::Pt4List jhmr::ReadLandsAsPtCloudH5Pt4(const H5::CommonFG& h5)
{
  return ReadLandsAsPtCloudH5Helper<Pt4>(h5);
}

jhmr::Pt2List
jhmr::ReadListOfPointsFromMatrixH5Pt2(const std::string& field_name, const H5::CommonFG& h5)
{
  return ReadListOfPointsFromMatrixH5Helper<CoordScalar,2>(field_name, h5);
}

jhmr::Pt3List
jhmr::ReadListOfPointsFromMatrixH5Pt3(const std::string& field_name, const H5::CommonFG& h5)
{
  return ReadListOfPointsFromMatrixH5Helper<CoordScalar,3>(field_name, h5);
}

jhmr::Pt4List
jhmr::ReadListOfPointsFromMatrixH5Pt4(const std::string& field_name, const H5::CommonFG& h5)
{
  return ReadListOfPointsFromMatrixH5Helper<CoordScalar,4>(field_name, h5);
}

jhmr::PtNList
jhmr::ReadListOfPointsFromMatrixH5PtN(const std::string& field_name, const H5::CommonFG& h5)
{
  return ReadListOfPointsFromMatrixH5Helper<CoordScalar>(field_name, h5);
}

std::vector<std::array<jhmr::size_type,3>>
jhmr::ReadListOfArraysFromMatrixH5Sizes3(const std::string& field_name, const H5::CommonFG& h5)
{
  return ReadListOfArraysFromMatrixH5Helper<size_type,3>(field_name, h5);
}

std::string jhmr::ReadStringH5(const std::string& field_name,
                               const H5::CommonFG& h5)
{
  const H5::DataSet data_set = h5.openDataSet(field_name);
  
  std::string s;
  data_set.read(s, data_set.getStrType());

  return s;
}

std::vector<std::string> jhmr::GetH5ObjNames(const H5::CommonFG& h5)
{
  const hsize_t num_objs = h5.getNumObjs();

  std::vector<std::string> names;
  names.reserve(num_objs);
  
  for (hsize_t i = 0; i < num_objs; ++i)
  {
    names.push_back(h5.getObjnameByIdx(i));
  }

  return names;
}

bool jhmr::ObjectInGroupH5(const std::string& obj_name, const H5::CommonFG& h5)
{
  const hsize_t num_objs = h5.getNumObjs();

  bool obj_exists = false;
  
  for (hsize_t i = 0; (i < num_objs) && !obj_exists; ++i)
  {
    if (obj_name == h5.getObjnameByIdx(i))
    {
      obj_exists = true;
    }
  }
  
  return obj_exists;
}

H5::Group jhmr::OpenOrCreateGroupH5(const std::string& group_name, H5::CommonFG* h5)
{
  return ObjectInGroupH5(group_name, *h5) ? h5->openGroup(group_name) : h5->createGroup(group_name);
}

std::tuple<std::vector<std::string>,std::vector<std::string>>
jhmr::GetH5GroupAndDatasetNames(const H5::CommonFG& h5)
{
  const hsize_t num_objs = h5.getNumObjs();

  std::vector<std::string> g_names;
  g_names.reserve(num_objs);
  
  std::vector<std::string> ds_names;
  ds_names.reserve(num_objs);
  
  for (hsize_t i = 0; i < num_objs; ++i)
  {
    const std::string n = h5.getObjnameByIdx(i);

    const auto t = h5.childObjType(n);

    if (t == H5O_TYPE_GROUP)
    {
      g_names.push_back(n);
    }
    else if (t == H5O_TYPE_DATASET)
    {
      ds_names.push_back(n);
    }
    else
    {
      jhmrThrow("UNKNOWN/UNEXPECTED H5O Type! (%d)", static_cast<int>(t));
    }
  }

  return std::make_tuple(g_names, ds_names);
}

jhmr::HideH5ExceptionPrints::HideH5ExceptionPrints()
{
  H5::Exception::getAutoPrint(auto_print_fn, &auto_print_data);
  H5::Exception::dontPrint();
}

jhmr::HideH5ExceptionPrints::~HideH5ExceptionPrints()
{
  H5::Exception::setAutoPrint(auto_print_fn, auto_print_data);
}
