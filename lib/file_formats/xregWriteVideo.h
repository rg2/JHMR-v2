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

#ifndef XREGWRITEVIDEO_H_
#define XREGWRITEVIDEO_H_

#include <vector>
#include <memory>

#include <opencv2/opencv.hpp>

namespace xreg
{

class WriteImageFramesToVideo
{
public:
  std::string dst_vid_path;

  double fps = 10;

  virtual void open() = 0;

  virtual void close() = 0;

  virtual void write(const cv::Mat& frame) = 0;

  virtual void write(const std::vector<cv::Mat>& frames);

  virtual ~WriteImageFramesToVideo() = default;
};

namespace detail
{

struct BoostProc;

}

class WriteImageFramesToVideoWithFFMPEG : public WriteImageFramesToVideo
{
public:
  WriteImageFramesToVideoWithFFMPEG();

  explicit WriteImageFramesToVideoWithFFMPEG(const std::string& ffmpeg_path_arg);

  // lower is higher quality, 0 is lossless, 17 should "appear" lossless
  double quality = 17;

  void open() override;

  void close() override;

  void write(const cv::Mat& frame) override;

  ~WriteImageFramesToVideoWithFFMPEG();
  
private:
  // shared_ptr can handle an incomplete type, unique cannot
  // using an incomplete type here, so I do not have to include
  // boost process in this header and define the few macros that
  // are required to make it header only - this can be done in the .cpp
  std::shared_ptr<detail::BoostProc> p;

  std::vector<unsigned char> png_buf;

  std::string ffmpeg_path;
};

class WriteImageFramesToVideoWithOpenCV : public WriteImageFramesToVideo
{
public:
  void open() override;

  void close() override;

  void write(const cv::Mat& frame) override;

private:
  std::unique_ptr<cv::VideoWriter> writer;
};

std::unique_ptr<WriteImageFramesToVideo> GetWriteImageFramesToVideo();

void WriteAllImageFramesToVideo(const std::string& vid_path,
                                const std::vector<cv::Mat>& frames,
                                const double fps = 10.0);

}  // xreg

#endif

