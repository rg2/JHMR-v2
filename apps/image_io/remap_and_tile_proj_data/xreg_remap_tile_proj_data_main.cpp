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

#include <numeric> // std::iota
#include <unordered_set>

#include <opencv2/imgcodecs.hpp>

#include "xregProgOptUtils.h"
#include "xregStringUtils.h"
#include "xregOpenCVUtils.h"
#include "xregITKOpenCVUtils.h"
#include "xregH5ProjDataIO.h"
#include "xregCSVUtils.h"

int main(int argc, char* argv[])
{
  constexpr int kEXIT_VAL_SUCCESS  = 0;
  constexpr int kEXIT_VAL_BAD_USE  = 1;
  constexpr int kEXIT_VAL_NO_PROJS = 2;

  using namespace xreg;

  using ImageUInt82D        = itk::Image<unsigned char,2>;
  using ImageUInt82DPointer = ImageUInt82D::Pointer;
  using ImageUInt82DList    = std::vector<ImageUInt82DPointer>;

  using OCVImgList = std::vector<cv::Mat>;
  using StringList = std::vector<std::string>;

  ProgOpts po;

  xregPROG_OPTS_SET_COMPILE_DATE(po);

  po.set_help("Given a collection of input projections, remaps each image and writes in a tiled image.");
  po.set_arg_usage("<Proj. Data File> <Output Tiled Image>");
  po.set_min_num_pos_args(2);

  po.add("width", 'w', ProgOpts::kSTORE_UINT32, "width",
         "The maximum width along the major direction, default direction is rows, "
         "so the default behavior is to write a 1 x num images set of tiles. "
         "When there are 5 images and the width is 2 and the direction is row, "
         "then there will be two rows, the first row with 3 tiles and the second "
         "row with 2 tiles.")
    << ProgOpts::uint32(1);

  po.add("col", 'c', ProgOpts::kSTORE_TRUE, "col",
         "Dimension with control over width is columns, default is rows.")
    << false;
  
  po.add("overlay-lands", 'o', ProgOpts::kSTORE_TRUE, "overlay-lands",
         "Overlay landmark locations onto the output images.")
    << false;

  po.add("overlay-lands-txt", ProgOpts::kNO_SHORT_FLAG, ProgOpts::kSTORE_STRING, "overlay-lands-txt",
         "Path to a text file which specifies the names of the landmarks that should be overlaid. "
         "Each line specifies the landmark names in a CSV format. "
         "Line 1 corresponds to projection 1, line 2 to projection 2, and so forth. "
         "When not provided, all landmarks are overlaid. "
         "\"overlay-lands\" (\'o\') must also be specified.")
    << "";

  po.add("overlay-color", ProgOpts::kNO_SHORT_FLAG, ProgOpts::kSTORE_STRING, "overlay-color",
         "Specify the default overlay color as a color name or MATLAB style color single character.")
    << "yellow";

  po.add("overlay-color-txt", ProgOpts::kNO_SHORT_FLAG, ProgOpts::kSTORE_STRING, "overlay-color-txt",
         "Path to a text file which specifies overlay colors for each landmark. "
         "Each line specifies a single landmark and is comma delimited with the landmark name first, "
         "followed by the color name. When a landmark is not specified then the color specified "
         "by \"overlay-color\" is used.")
    << "";
  
  const std::unordered_map<std::string,cv::MarkerTypes> kMARKER_NAME_TO_ENUM = {
    { "cross",    cv::MARKER_CROSS         },
    { "x",        cv::MARKER_TILTED_CROSS  },
    { "star",     cv::MARKER_STAR          },
    { "diamond",  cv::MARKER_DIAMOND       },
    { "square",   cv::MARKER_SQUARE        },
    { "tri-up",   cv::MARKER_TRIANGLE_UP   },
    { "tri-down", cv::MARKER_TRIANGLE_DOWN }
  };
  
  {
    std::stringstream overlay_shape_help_ss;

    overlay_shape_help_ss << "Specify the default overlay shape used for landmarks.";
    overlay_shape_help_ss << " Possible shape values are: ";
    for (const auto& marker_kv : kMARKER_NAME_TO_ENUM)
    {
      overlay_shape_help_ss << fmt::sprintf("\"%s,\" ", marker_kv.first);
    }
    overlay_shape_help_ss << "and \"circle.\"";

    po.add("overlay-shape", ProgOpts::kNO_SHORT_FLAG, ProgOpts::kSTORE_STRING, "overlay-shape",
           overlay_shape_help_ss.str())
      << "circle";
  }

  po.add("overlay-shape-txt", ProgOpts::kNO_SHORT_FLAG, ProgOpts::kSTORE_STRING, "overlay-shape-txt",
         "Path to text file that specifies the type of shape to use for specific landmarks. "
         "Each line specifies a single landmark and is comma delimited with the landmark name first, "
         "followed by the shape name. When a landmark is not specified then the shape specified "
         "by \"overlay-shape\" is used.")
    << "";

  po.add("border-thickness", 'b', ProgOpts::kSTORE_INT32, "border-thickness",
         "Thickness of the rectangle borders drawn to separate projections. "
         "Thickness <= 0 does not draw a border.")
    << ProgOpts::int32(5);

  po.add("projs", 'p', ProgOpts::kSTORE_STRING, "projs",
         "Comma delimited list of zero-based projection indices and ranges to put "
         "in the tiled output image. \"\" --> use all projections.")
    << "";

  po.add("ds-factor", 'd', ProgOpts::kSTORE_DOUBLE, "ds-factor",
         "Downsampling factor applied to projection data")
    << 1.0;
  
  po.add("no-pat-rot-up", ProgOpts::kNO_SHORT_FLAG, ProgOpts::kSTORE_TRUE, "no-pat-rot-up",
         "Ignore any flags for rotating the image to achieve patient \"up\" orientation. ")
    << false;

  po.add("flip-rows", ProgOpts::kNO_SHORT_FLAG, ProgOpts::kSTORE_TRUE, "flip-rows",
         "Flip the rows of each image before tiling. "
         "Unless the \"no-pat-rot-up\" flag is passed, this flag is automatically "
         "overriden based on metadata within the projection file indicating patient "
         "up orientation (if the metadata is present).")
    << false;

  po.add("flip-cols", ProgOpts::kNO_SHORT_FLAG, ProgOpts::kSTORE_TRUE, "flip-cols",
         "Flip the columns of each image before tiling. "
         "Unless the \"no-pat-rot-up\" flag is passed, this flag is automatically "
         "overriden based on metadata within the projection file indicating patient "
         "up orientation (if the metadata is present).")
    << false;

  try
  {
    po.parse(argc, argv);
  }
  catch (const ProgOpts::Exception& e)
  {
    std::cerr << "Error parsing command line arguments: " << e.what() << std::endl;
    po.print_usage(std::cerr);
    return kEXIT_VAL_BAD_USE;
  }

  if (po.help_set())
  {
    po.print_usage(std::cout);
    po.print_help(std::cout);
    return kEXIT_VAL_SUCCESS;
  }

  std::ostream& vout = po.vout();

  const ProgOpts::uint32 width = po.get("width");

  const bool tile_as_col = po.get("col");
  
  const bool no_pat_rot_up = po.get("no-pat-rot-up");

  const bool flip_rows_flag = po.get("flip-rows");
  const bool flip_cols_flag = po.get("flip-cols");

  const bool overlay_lands = po.get("overlay-lands");

  const ProgOpts::int32 border_thickness = po.get("border-thickness");

  const std::string projs_str = po.get("projs");

  const std::string overlay_lands_txt_path = po.get("overlay-lands-txt");

  const bool overlay_all_lands = overlay_lands_txt_path.empty();

  std::vector<std::unordered_set<std::string>> lands_to_show;

  if (overlay_lands)
  {
    if (overlay_all_lands)
    {
      vout << "will overlay all landmarks." << std::endl;
    }
    else
    {
      vout << "reading landmarks to show from txt file..." << std::endl;

      std::ifstream fin(overlay_lands_txt_path);

      const auto lines = GetNonEmptyLinesFromStream(fin);

      lands_to_show.reserve(lines.size());

      for (const auto& l : lines)
      {
        const auto toks = StringSplit(l, ",");

        lands_to_show.emplace_back(toks.begin(), toks.end());
      }
    }
  }

  const auto& ocv_color_lut = OpenCVColorNameToScalar();

  const std::string overlay_color_str = po.get("overlay-color");

  const auto default_overlay_color = ocv_color_lut.find(overlay_color_str)->second;

  const std::string landmark_color_txt_path = po.get("overlay-color-txt");

  std::unordered_map<std::string,cv::Scalar> land_color_lut;
  
  if (!landmark_color_txt_path.empty())
  {
    const auto csv_rows = ReadCSVFileStr(landmark_color_txt_path, false);

    for (const auto& csv_row : csv_rows)
    {
      xregASSERT(csv_row.size() == 2);
      land_color_lut.emplace(csv_row[0], ocv_color_lut.find(csv_row[1])->second);
    }
  }

  const std::string overlay_shape_str = po.get("overlay-shape");

  const std::string overlay_shape_txt_path = po.get("overlay-shape-txt");

  std::unordered_map<std::string,std::string> landmark_shape_name_lut;
  if (!overlay_shape_txt_path.empty())
  {
    const auto csv_rows = ReadCSVFileStr(overlay_shape_txt_path, false);

    for (const auto& csv_row : csv_rows)
    {
      xregASSERT(csv_row.size() == 2);
      landmark_shape_name_lut.emplace(csv_row[0], csv_row[1]);
    }
  }

  const double proj_ds_factor = po.get("ds-factor");

  const std::string proj_data_path = po.pos_args()[0];
  const std::string tiled_img_path = po.pos_args()[1];

  vout << "reading projection data..." << std::endl;
  auto pd = ReadProjDataH5F32FromDisk(proj_data_path);
  
  if (std::abs(proj_ds_factor - 1.0) > 0.001)
  {
    vout << "  downsampling projection data..." << std::endl;
    pd = DownsampleProjData(pd, proj_ds_factor);
  }

  const size_type num_src_projs = pd.size();
  vout << "number of cameras/projections in file: " << num_src_projs << std::endl;

  std::vector<long> projs_to_use;
  
  if (projs_str.empty())
  {
    projs_to_use.resize(num_src_projs);
    std::iota(projs_to_use.begin(), projs_to_use.end(), 0);
  }
  else
  {
    projs_to_use = ParseCSVRangeOfInts(projs_str);
  }

  const size_type num_projs = projs_to_use.size();
  vout << "number of cameras/projections to use: " << num_projs << std::endl;

  if (num_projs)
  {
    ImageUInt82DList remapped_projs(num_projs);
    OCVImgList ocv_imgs(num_projs);

    for (size_type proj_idx = 0; proj_idx < num_projs; ++proj_idx)
    {
      const size_type src_proj_idx = static_cast<size_type>(projs_to_use[proj_idx]);
      xregASSERT(src_proj_idx < num_src_projs);
      
      const auto& src_pd = pd[src_proj_idx];

      vout << "remapping " << src_proj_idx << " to 8bpp..." << std::endl;
      remapped_projs[proj_idx] = ITKImageRemap8bpp(src_pd.img.GetPointer());
      ocv_imgs[proj_idx] = ShallowCopyItkToOpenCV(remapped_projs[proj_idx].GetPointer());

      if (overlay_lands)
      {
        cv::Mat& cur_img = ocv_imgs[proj_idx];

        // convert to color so overlays are in color
        cv::Mat bgr_img;
        cv::cvtColor(cur_img, bgr_img, cv::COLOR_GRAY2BGR);
        cur_img = bgr_img;

        cv::Point tmp_pt;

        const size_type lands_to_show_idx = lands_to_show.empty() ? 0 : (proj_idx % lands_to_show.size());

        for (const auto& lkv : src_pd.landmarks)
        {
          if (overlay_all_lands ||
              (lands_to_show[lands_to_show_idx].find(lkv.first) != lands_to_show[lands_to_show_idx].end()))
          {
            const auto land_color_lut_it = land_color_lut.find(lkv.first);
            
            const auto overlay_color = (land_color_lut_it != land_color_lut.end()) ?
                                          land_color_lut_it->second : default_overlay_color;

            tmp_pt.x = std::lround(lkv.second(0));
            tmp_pt.y = std::lround(lkv.second(1));
        
            int radius = 20;
            
            constexpr int thickness = 3;

            std::string cur_overlay_shape_str = overlay_shape_str;

            const auto cur_shape_str_it = landmark_shape_name_lut.find(lkv.first);
            if (cur_shape_str_it != landmark_shape_name_lut.end())
            {
              cur_overlay_shape_str = cur_shape_str_it->second;
            }

            const auto marker_it = kMARKER_NAME_TO_ENUM.find(cur_overlay_shape_str);

            if ((marker_it == kMARKER_NAME_TO_ENUM.end()) || (cur_overlay_shape_str == "circle"))
            {
              cv::circle(cur_img, tmp_pt, radius, overlay_color, thickness);
            }
            else
            {
              const auto& marker_enum = marker_it->second;

              if ((marker_enum != cv::MARKER_TILTED_CROSS) ||
                  (marker_enum != cv::MARKER_DIAMOND))
              {
                radius *= 2;
              }

              cv::drawMarker(cur_img, tmp_pt, overlay_color, marker_it->second, radius, thickness);
            }
          }
        }
      }

      bool flip_rows = flip_rows_flag;
      bool flip_cols = flip_cols_flag;

      if (!no_pat_rot_up && src_pd.rot_to_pat_up)
      {
        const auto& rot_to_pat_up = *src_pd.rot_to_pat_up;

        if (rot_to_pat_up == ProjDataF32::kZERO)
        {
          flip_rows = false;
          flip_cols = false;
        }
        else if (rot_to_pat_up == ProjDataF32::kONE_EIGHTY)
        {
          flip_rows = true;
          flip_cols = true;
        }
        else
        {
          vout << "ignoring rot_to_pat_up value of: " << static_cast<int>(rot_to_pat_up) << std::endl;
        }
      }

      if (flip_rows)
      {
        vout << "  flipping rows..." << std::endl;
        FlipImageRows(&ocv_imgs[proj_idx]);
      }

      if (flip_cols)
      {
        vout << "  flipping cols..." << std::endl;
        FlipImageColumns(&ocv_imgs[proj_idx]);
      }
    }

    vout << "tiling..." << std::endl;

    size_type num_rows = 0;
    size_type num_cols = 0;

    if (tile_as_col)
    {
      num_rows = num_projs / width;
      if (num_projs % width)
      {
        ++num_rows;
      }
      num_cols = width;
    }
    else
    {
      num_cols = num_projs / width;
      if (num_projs % width)
      {
        ++num_cols;
      }
      num_rows = width;
    }

    OCVImgList tiles = CreateSummaryTiledImages(ocv_imgs, StringList(),
                                                num_rows, num_cols,
                                                border_thickness);
    xregASSERT(tiles.size() == 1);

    vout << "writing output tile..." << std::endl;
    cv::imwrite(tiled_img_path, tiles[0]);
  }
  else
  {
    std::cerr << "ERROR: NO PROJECTIONS IN FILE!" << std::endl;
    return kEXIT_VAL_NO_PROJS;
  }

  vout << "exiting..." << std::endl;
  return kEXIT_VAL_SUCCESS;
}
