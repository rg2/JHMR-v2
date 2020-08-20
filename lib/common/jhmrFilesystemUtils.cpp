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

#include "jhmrFilesystemUtils.h"

#include <algorithm>
#include <fstream>

// access() includes
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

// stat() includes
#ifdef _WIN32
#include <sys/types.h>
#endif
#include <sys/stat.h>

// FindFirstFile, dirent includes
#ifdef _WIN32
#include <Windows.h>
#include <direct.h>
#else
#include <dirent.h>
#endif

// for PATH_MAX on linux
#if !defined(_WIN32) && !defined(__APPLE__)
#include <climits>
#endif

#include <boost/algorithm/string.hpp>

#include <fmt/printf.h>

#include "jhmrAssert.h"
#include "jhmrStringUtils.h"

#include "jhmrSampleUtils.h"

#ifdef _WIN32
static char _file_seps_arr[2] = { jhmr::kWIN_FILE_SEP, jhmr::kPOSIX_FILE_SEP };
const char* jhmr::kFILE_SEPS = _file_seps_arr;
#else
const char* jhmr::kFILE_SEPS = &jhmr::kPOSIX_FILE_SEP;
#endif

#ifdef _WIN32
typedef struct __stat64 stat_type;
#define STAT_FN _stat64
#else
typedef struct stat stat_type;
#define STAT_FN stat
#endif

bool jhmr::IsHDF5FileExt(const std::string& path)
{
  const std::string ext_lower = ToLowerCase(GetFileExtension(path));

  return (ext_lower == ".h5") || (ext_lower == ".hdf5");
}

jhmr::FileSystemException::FileSystemException() throw()
{
  set_msg("Filesystem Exception");
}

jhmr::FileSystemException::FileSystemException(const char* path, const char* msg) throw()
{
  set_msg("Filesystem Exception: %s, %s", path, msg);
}

jhmr::Path::Path()
{ }

jhmr::Path::Path(const std::string& path_str)
  : path_str_(path_str)
{ }

jhmr::Path::Path(const char* path_str)
  : path_str_(path_str)
{ }

jhmr::Path jhmr::Path::filename() const
{
  return Path(GetFileName(path_str_));
}

jhmr::Path jhmr::Path::parent() const
{
  const char* kREL_PARENT_STR = "..";

  std::string parent_str;

  if (!path_str_.empty())
  {
    size_type sep_pos[kNUM_FILE_SEPS];

    for (size_type i = 0; i < kNUM_FILE_SEPS; ++i)
    {
      sep_pos[i] = path_str_.find_last_of(kFILE_SEPS[i]);
    }
    
    // Determine which path separator is last
    size_type last_sep_pos = std::string::npos;

    if (kNUM_FILE_SEPS == 1)
    {
      last_sep_pos = sep_pos[0];
    }
    else
    {
      std::for_each(sep_pos, sep_pos + kNUM_FILE_SEPS,
                    [&last_sep_pos](const size_type idx)
                    {
                      if ((last_sep_pos == std::string::npos) ||
                          ((idx != std::string::npos) && (idx > last_sep_pos)))
                      {
                        last_sep_pos = idx;
                      }
                    });
    }

    if (last_sep_pos != std::string::npos)
    {
      // Path separator was found
      parent_str = path_str_.substr(0,last_sep_pos);
    }
    else if (is_dir())
    {
      // A path separator was not found, and the current path represents a
      // directory, so return relative parent <current path>/..
      parent_str = path_str_;
      parent_str.push_back(kFILE_SEPS[0]);
      parent_str += kREL_PARENT_STR;
    }
    else
    {
      // A path separator was not found, and the current path is a file,
      // therefore it is a relative path, so return the relative parent ".."
      parent_str = kREL_PARENT_STR;
    }
  }
  else
  {
    // else the path is empty, return relative parent ".."
    parent_str = kREL_PARENT_STR;
  }

  return Path(parent_str);
}

std::string jhmr::Path::file_extension() const
{
  return GetFileExtension(path_str_);
}

void jhmr::Path::split_ext(std::string* prefix, std::string* ext) const
{
  SplitPathPrefixExtension(path_str_, prefix, ext);
}

void jhmr::Path::split_ext_w_comp(std::string* prefix, std::string* ext) const
{
  // Just off the top of my head right now
  const std::string kCOMP_EXTS[3] = { ".gz", ".bz2", ".xz" };

  std::string cur_prefix;
  std::string cur_ext;

  split_ext(&cur_prefix, &cur_ext);

  if (std::find(kCOMP_EXTS, kCOMP_EXTS + 3, ToLowerCase(cur_ext)) != (kCOMP_EXTS + 3))
  {
    std::string path_str_wo_comp_ext = cur_prefix;
    std::string comp_ext = cur_ext;
    SplitPathPrefixExtension(path_str_wo_comp_ext, &cur_prefix, &cur_ext);
    cur_ext += comp_ext;
  }

  if (prefix)
  {
    *prefix = cur_prefix;
  }

  if (ext)
  {
    *ext = cur_ext;
  }
}

jhmr::Path jhmr::Path::prefix() const
{
  std::string s;

  split_ext(&s, 0);

  return s;
}

bool jhmr::Path::exists() const
{
#ifdef _WIN32
  return _access(path_str_.c_str(), 00) == 0;
#else
  return access(path_str_.c_str(), F_OK) == 0;
#endif

}

bool jhmr::Path::is_dir() const
{
  bool b = false;

  stat_type s;
  if (STAT_FN(path_str_.c_str(), &s) == 0)
  {
    b = (s.st_mode & S_IFDIR) != 0;
  }
  else
  {
    throw FileSystemException(path_str_.c_str(), "jhmr::Path::is_dir: Failed to stat file path!");
  }

  return b;
}

bool jhmr::Path::is_reg_file() const
{
  bool b = false;

  stat_type s;
  if (STAT_FN(path_str_.c_str(), &s) == 0)
  {
    b = (s.st_mode & S_IFREG) != 0;
  }
  else
  {
    throw FileSystemException(path_str_.c_str(), "jhmr::Path::is_reg_file: Failed to stat file path!");
  }

  return b;
}

bool jhmr::Path::is_exec() const
{
  bool b = false;

  stat_type s;
  if (STAT_FN(path_str_.c_str(), &s) == 0)
  {
    b = (s.st_mode & (S_IFREG | S_IXUSR)) != 0;
  }
  else
  {
    throw FileSystemException(path_str_.c_str(), "jhmr::Path::is_exec: Failed to stat file path!");
  }

  return b;
}

bool jhmr::Path::has_trailing_sep() const
{
  bool found_trailing_sep = false;

  if (!path_str_.empty())
  {
    const char last_char = *--path_str_.end();

    for (unsigned long i = 0; !found_trailing_sep && (i < kNUM_FILE_SEPS); ++i)
    {
      found_trailing_sep = last_char == kFILE_SEPS[i];
    }
  }

  return found_trailing_sep;
}

void jhmr::Path::append(const Path& p)
{
  // in case this == &p
  const std::string other_str = p.string();

  if (!path_str_.empty() && !has_trailing_sep())
  {
    path_str_.push_back(kFILE_SEPS[0]);
  }

  path_str_ += other_str;
}

jhmr::Path& jhmr::Path::operator+=(const Path& rhs)
{
  append(rhs);
  return *this;
}

void jhmr::Path::get_dir_contents(PathList* paths, const bool filenames_only) const
{
#ifdef _WIN32
  // adapted from http://msdn.microsoft.com/en-us/library/aa365200%28VS.85%29.aspx

  std::string query_str = path_str_;
  if (!has_trailing_sep())
  {
    query_str.push_back(kFILE_SEPS[0]);
  }
  query_str.push_back('*');

  WIN32_FIND_DATA ffd;

  HANDLE hFind = FindFirstFile(query_str.c_str(), &ffd);
  if (hFind != INVALID_HANDLE_VALUE)
  {
    Path tmp_path;
    std::string cur_name;

    do
    {
      cur_name.assign(reinterpret_cast<const char*>(ffd.cFileName));

      if (!filenames_only)
      {
        tmp_path = *this;
        tmp_path += cur_name;
      }
      else
      {
        tmp_path = cur_name;
      }

      paths->push_back(tmp_path);
    }
    while (FindNextFile(hFind, &ffd) != 0);

    DWORD last_err = GetLastError();

    FindClose(hFind);

    if (last_err != ERROR_NO_MORE_FILES)
    {
      throw FileSystemException(path_str_.c_str(), "jhmr::Path::get_dir_contents: FindNextFile() error!");
    }
  }
  else
  {
    throw FileSystemException(path_str_.c_str(), "jhmr::Path::get_dir_contents: FindFirstFile() failure!");
  }
#else
  DIR* dir = opendir(path_str_.c_str());
  if (dir != 0)
  {
    Path tmp_path;

    struct dirent* dir_ent = 0;
    while ((dir_ent = readdir(dir)) != 0)
    {
      if (!filenames_only)
      {
        tmp_path = *this;
        tmp_path += dir_ent->d_name;
      }
      else
      {
        tmp_path = dir_ent->d_name;
      }

      paths->push_back(tmp_path);
    }

    closedir(dir);
  }
  else
  {
    throw FileSystemException(path_str_.c_str(),
                              "jhmr::Path::get_dir_contents: opendir() failure!");
  }
#endif
}

void jhmr::Path::set_posix_root()
{
  path_str_.resize(1);
  path_str_[0] = kPOSIX_FILE_SEP;
}

jhmr::Path jhmr::Path::absolute_path() const
{
  std::vector<char> tmp_path_str;

#ifdef _WIN32
  tmp_path_str.resize(1024);
  if (!_fullpath(&tmp_path_str[0], path_str_.c_str(), tmp_path_str.size()))
  {
    throw FileSystemException(path_str_.c_str(),
                              "jhmr::Path::absolute_path: _fullpath() failure!");
  }
#else
  tmp_path_str.resize(PATH_MAX);
  if (!realpath(path_str_.c_str(), &tmp_path_str[0]))
  {
    throw FileSystemException(path_str_.c_str(),
                              "jhmr::Path::absolute_path: realpath() failure!");
  } 
#endif

  return Path(&tmp_path_str[0]);
}

std::string jhmr::ConvertToWinPath(const std::string& path)
{
  std::string new_str = path;

  std::replace(new_str.begin(), new_str.end(), kPOSIX_FILE_SEP, kWIN_FILE_SEP);

  return new_str;
}

std::string jhmr::ConvertToPosixPath(const std::string& path)
{
  std::string new_str = path;

  std::replace(new_str.begin(), new_str.end(), kWIN_FILE_SEP, kPOSIX_FILE_SEP);

  return new_str;
}

std::string jhmr::GetFileName(const std::string& path)
{
  using size_type = std::string::size_type;

  const size_type path_str_len = path.size();

  std::string name;

  if (!path.empty())
  {
    size_type sep_pos[kNUM_FILE_SEPS];

    for (size_type i = 0; i < kNUM_FILE_SEPS; ++i)
    {
      sep_pos[i] = path.find_last_of(kFILE_SEPS[i]);
    }

    bool sep_found = false;
    size_type cur_max_last_sep_pos = 0;

    if (kNUM_FILE_SEPS == 1)
    {
      sep_found = sep_pos[0] != std::string::npos;
      cur_max_last_sep_pos = sep_pos[0];
    }
    else
    {
      for (size_type sep_idx = 0; sep_idx < kNUM_FILE_SEPS; ++sep_idx)
      {
        if (sep_pos[sep_idx] != std::string::npos)
        {
          sep_found = true;
          cur_max_last_sep_pos = std::max(cur_max_last_sep_pos, sep_pos[sep_idx]);
        }
      }
    }

    const size_type last_sep_pos = sep_found ? cur_max_last_sep_pos : std::string::npos;

    if (last_sep_pos == std::string::npos)
    {
      // A path seperator was not found, the entire path is a filename
      name = path;
    }
    else if (last_sep_pos < (path_str_len - 1))
    {
      // The last path separator is not at the end of the path,
      // indicating that the path actually has a file component.
      name = path.substr(last_sep_pos + 1);
    }
  }

  return name;
}

std::string jhmr::GetFileExtension(const std::string& path)
{
  using size_type = std::string::size_type;

  std::string ext_str;

  const std::string filename = GetFileName(path);

  if (!filename.empty())
  {
    const size_type last_dot_pos = filename.find_last_of('.');

    if ((last_dot_pos != std::string::npos) && (last_dot_pos < (filename.size() - 1)))
    {
      // a dot/period was found and it is not the last character of the filename
      ext_str = filename.substr(last_dot_pos);
    }
  }

  return ext_str;
}

void jhmr::SplitPathPrefixExtension(const std::string& path, std::string* prefix,
                                  std::string* ext)
{
  std::string p;
  std::string e;

  if (!path.empty())
  {
    const std::string::size_type last_dot_pos = path.find_last_of('.');

    p.assign(path.substr(0, last_dot_pos));

    if (last_dot_pos != std::string::npos)
    {
      e.assign(path.substr(last_dot_pos));
    }
    else
    {
      e.assign("");
    }
  }
  else
  {
    p.assign("");
    e.assign("");
  }

  if (prefix)
  {
    prefix->swap(p);
  }

  if (ext)
  {
    ext->swap(e);
  }
}

void jhmr::FileExtensions::add(const std::string& e)
{
  jhmrASSERT((e.size() > 1) && (e[0] == '.'));

  std::string e_lower = e;
  boost::algorithm::to_lower(e_lower);

  exts_.insert(e_lower);
}

bool jhmr::FileExtensions::check_file(const Path& p) const
{
  std::string ext_lower = p.file_extension();

  boost::algorithm::to_lower(ext_lower);

  return exts_.find(ext_lower) != exts_.end();
}

void jhmr::GetFilePathsFromDir(const Path& p, PathStringList* files, const FileExtensions& exts)
{
  if (p.exists() && p.is_dir())
  {
    std::string tmp_str;

    PathList dir_contents;

    p.get_dir_contents(&dir_contents);

    for (PathList::iterator dir_it = dir_contents.begin(); dir_it != dir_contents.end(); ++dir_it)
    {
      if (dir_it->is_reg_file())
      {
        if (exts.check_file(*dir_it))
        {
          files->push_back(dir_it->string());
        }
      }
    }
  }
  else
  {
    throw FileSystemException(p.string().c_str(), "Invalid Directory! Cannot list contents!");
  }
}

void jhmr::GetFilePathsFromDir(const Path& p, PathStringList* files, const std::string& ext)
{
  FileExtensions e;
  e.add(ext);
  GetFilePathsFromDir(p, files, e);
}

void jhmr::GetFilePathsFromDir(const Path& p, PathStringList* files)
{
  GetFilePathsFromDir(p, files, AllowAllFileExtensions());
}

void jhmr::GetFilePathsFromDirWithSuffix(const Path& p, PathStringList* files,
                                         const std::string& suffix,
                                         const bool case_sensitive)
{
  const std::string suffix_to_use = case_sensitive ? suffix : ToUpperCase(suffix);

  PathStringList tmp_files;

  GetFilePathsFromDir(p, &tmp_files);

  files->clear();
  files->reserve(tmp_files.size());

  for (PathStringList::const_iterator tmp_it = tmp_files.begin();
       tmp_it != tmp_files.end(); ++tmp_it)
  {
    const std::string& cur_path = *tmp_it;

    if (StringEndsWith(case_sensitive ? cur_path : ToUpperCase(cur_path),
                       suffix_to_use))
    {
      files->push_back(*tmp_it);
    }
  }
}

jhmr::PathStringList jhmr::GetNonEmptyLinesFromStream(std::istream& in)
{
  PathStringList lines;
  
  std::string cur_line;

  while (in.good())
  {
    std::getline(in, cur_line);
    boost::algorithm::trim(cur_line);

    if (!cur_line.empty())
    {
      lines.push_back(cur_line);
    }
  }
  
  return lines;
}

jhmr::ScopedCFile::ScopedCFile(const char* path, const char* mode)
{
  fp_ = fopen(path, mode);
  if (!fp_)
  {
    // TODO: check errno and throw a more specific exception
    std::ostringstream oss;
    oss << "fopen failed for mode: " << mode;
    throw FileSystemException(path, oss.str().c_str());
  }
}

jhmr::ScopedCFile::~ScopedCFile()
{
  close();
}

void jhmr::ScopedCFile::close()
{
  if (fp_)
  {
    // it is the user's responsibility to call fflush()
    fclose(fp_);
    fp_ = 0;
  }
}

jhmr::ScopedCFile::operator FILE*()
{
  return fp_;
}

jhmr::FileInputStream::FileInputStream(const std::string& path)
  : fp_(path.c_str(), "rb"), file_path_(path)
{ }

void jhmr::FileInputStream::raw_read(void* ptr,
                                    const InputStream::size_type num_bytes)
{
  if (fread(ptr, num_bytes, 1, fp_) != 1)
  {
    std::ostringstream oss;
    oss << "failed to read " << num_bytes << " bytes";
    throw FileSystemException(file_path_.c_str(), oss.str().c_str());
  }
}

jhmr::InputStream::size_type jhmr::FileInputStream::num_bytes_left()
{
  const long orig_pos = ftell(fp_);
  fseek(fp_, 0, SEEK_END);
  const long to_ret = ftell(fp_) - orig_pos;
  fseek(fp_, orig_pos, SEEK_SET);
  return to_ret;
}

void jhmr::FileInputStream::read_remaining_bytes(void* buf)
{
  raw_read(buf, num_bytes_left());
}

jhmr::FileOutputStream::FileOutputStream(const std::string& path)
  : fp_(path.c_str(), "wb"), file_path_(path)
{ }

jhmr::FileOutputStream::~FileOutputStream()
{
  fflush(fp_);
}

void jhmr::FileOutputStream::raw_write(const void* ptr,
                                    const FileOutputStream::size_type num_bytes)
{
  if (fwrite(ptr, num_bytes, 1, fp_) != 1)
  {
    std::ostringstream oss;
    oss << "failed to write " << num_bytes << " bytes";
    throw FileSystemException(file_path_.c_str(), oss.str().c_str());
  }
}

void jhmr::CopyFileSystemItem(const std::string& src_path, const std::string& dst_path)
{
  Path src_path_obj(src_path);

  if (src_path_obj.is_reg_file())
  {
    Path dst_path_obj(dst_path);

    if (dst_path_obj.exists() && dst_path_obj.is_dir())
    {
      dst_path_obj += src_path_obj.filename();
    }

    std::ifstream in(src_path.c_str(), std::ifstream::binary);
    if (in.is_open())
    {
      std::ofstream out(dst_path_obj.string().c_str(), std::ofstream::binary);
      if (out.is_open())
      {
        constexpr size_type kBUF_SIZE = 8192;  // 8 KB
        std::vector<char> buf(kBUF_SIZE);

        while (in.good())
        {
          in.read(&buf[0], kBUF_SIZE);
          out.write(&buf[0], in.gcount());
        }

        out.flush();
      }
      else
      {
        throw FileSystemException(dst_path_obj.string().c_str(), "Failed to open destination for writing!");
      }
    }
    else
    {
      throw FileSystemException(src_path.c_str(), "Failed to open source for reading!");
    }

  }
  else
  {
    throw FileSystemException(src_path.c_str(), "Source is not a regular file!");
  }
}

std::string jhmr::GetUniqueNumericIDPath(const std::string& path_pattern)
{
  using size_type = std::string::size_type;

  std::string cur_str;

  bool is_unique = false;

  for (size_type cur_id = 0; !is_unique; ++cur_id)
  {
    cur_str = fmt::sprintf(path_pattern, cur_id);

    is_unique = !Path(cur_str).exists();
  }

  return cur_str;
}

void jhmr::ExtractNonCommentLines(const PathStringList& src_paths, PathStringList* dst_paths,
                                const PathStringList& comment_strs)
{
  dst_paths->clear();

  std::string tmp_str;

  for (PathStringList::const_iterator src_it = src_paths.begin(); src_it != src_paths.end(); ++src_it)
  {
    tmp_str = *src_it;
    boost::algorithm::trim(tmp_str);

    bool is_cmt = false;
    for (PathStringList::const_iterator cmt_it = comment_strs.begin();
         !is_cmt && (cmt_it != comment_strs.end()); ++cmt_it)
    {
      is_cmt = tmp_str.find(*cmt_it) == 0;
    }

    if (!is_cmt)
    {
      dst_paths->push_back(*src_it);
    }
  }

}

void jhmr::ExtractNonCommentList(const PathStringList& src_paths, PathStringList* dst_paths,
                               const std::string& comment_str)
{
  PathStringList cmts;
  cmts.push_back(comment_str);
  ExtractNonCommentLines(src_paths, dst_paths, cmts);
}

void jhmr::GetNonEmptyNonCommentLinesFromStream(std::istream& in, PathStringList* paths,
                                              const std::string& comment_str)
{
  PathStringList lines = GetNonEmptyLinesFromStream(in);
  ExtractNonCommentList(lines, paths, comment_str);
}

void jhmr::MakeDir(const std::string& path)
{
  const Path dir_path(path);

  if (dir_path.exists())
  {
    if (!dir_path.is_dir())
    {
      throw FileSystemException(path.c_str(), "Path exists and is not a directory!");
    }

    return;
  }

#ifdef _WIN32
  if (_mkdir(path.c_str()))
  {
    char err_msg[2048];
    if (strerror_s(err_msg, sizeof(err_msg), errno) == 0)
    {
      throw FileSystemException(path.c_str(), err_msg);
    }
    else
    {
      throw FileSystemException(path.c_str(), "Directory creation failed!");
    }
  }
#else
  if (mkdir(path.c_str(), 0770))
  {
    char err_msg[2048];
    if (strerror_r(errno, err_msg, sizeof(err_msg)) == 0)
    {
      throw FileSystemException(path.c_str(), err_msg);
    }
    else
    {
      throw FileSystemException(path.c_str(), "Directory creation failed!");
    }
  }
#endif
}

void jhmr::MakeDirRecursive(const std::string& path)
{
  // convert to posix path so we can tokenize on a single separator
  const std::string posix_path = ConvertToPosixPath(path);

  char path_sep_str[2] = { kPOSIX_FILE_SEP, '\0' };
  const auto path_components = StringSplit(posix_path, path_sep_str);
  // by default the splitting will ignore empty tokens and we will lose whether
  // or not the path is absolute, we do a check below to handle this.

  size_type num_comps = path_components.size();

  if (num_comps > 0)
  {
    // correctly handle an absolute path (leading /)
    Path cur_path;
    if (path[0] == kPOSIX_FILE_SEP)
    {
      cur_path.set_posix_root();
    }

    cur_path += path_components[0];

    for (size_type comp_idx = 0; comp_idx < num_comps; ++comp_idx)
    {
      if (!cur_path.exists())
      {
        MakeDir(cur_path.string());
      }

      if (comp_idx < (num_comps - 1))
      {
        cur_path += path_components[comp_idx + 1];
      }
    }
  }
}

void jhmr::RemoveDirRecursive(const std::string& path)
{
  Path cur_path(path);
  
  if (cur_path.is_dir())
  {
    PathList child_paths;

    cur_path.get_dir_contents(&child_paths);
  
    for (Path& cur_child : child_paths)
    {
      const std::string cur_child_path_str = cur_child.string();
      
      const std::string base_name = GetFileName(cur_child_path_str);

      if ((base_name != ".") && (base_name != ".."))
      {
        if (cur_child.is_dir())
        {
          RemoveDirRecursive(cur_child_path_str);
        }
        else
        {
          if (std::remove(cur_child_path_str.c_str()))
          {
            throw FileSystemException(cur_child_path_str.c_str(), "failed to delete non-dir entry!");
          }
        }
      }
    }

#ifdef _WIN32
    if (!RemoveDirectoryA(path.c_str()))
#else
    if (rmdir(path.c_str()))
#endif
    {
      throw FileSystemException(path.c_str(), "directory deletion failed!");
    }
  }
  else
  {
    throw FileSystemException(path.c_str(), "cannot recursively remove non-directory!");
  }
}

void jhmr::MoveFileSystemItem(const std::string& src_path, const std::string& dst_path)
{
  Path src_path_obj(src_path);
  Path dst_path_obj(dst_path);

  if (dst_path_obj.exists() && dst_path_obj.is_dir())
  {
    dst_path_obj += src_path_obj.filename();
  }

#ifdef _WIN32
  if (!MoveFileEx(src_path.c_str(), dst_path_obj.string().c_str(),
                  MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
  {
    throw FileSystemException(src_path.c_str(), "Filesystem Move failed!");
  }
#else
  if (rename(src_path.c_str(), dst_path_obj.string().c_str()) != 0)
  {
    char err_msg[2048];
    if (strerror_r(errno, err_msg, sizeof(err_msg)) == 0)
    {
      throw FileSystemException(src_path.c_str(), err_msg);
    }
    else
    {
      throw FileSystemException(src_path.c_str(), "Filesystem Move failed!");
    }
  }
#endif
}

jhmr::Path jhmr::operator+(const Path& p1, const Path& p2)
{
  Path p = p1;
  p += p2;
  return p;
}

jhmr::CreateTempDir::CreateTempDir(const std::string& prefix)
{
  std::mt19937 rng_eng;
  SeedRNGEngWithRandDev(&rng_eng);

  while (true)
  {
    Path p(fmt::sprintf("%s_%X", prefix, static_cast<unsigned long>(rng_eng())));
    
    if (!p.exists())
    {
      MakeDirRecursive(p.string());
      
      p_ = p;

      break;
    }
  }
}
  
jhmr::CreateTempDir::~CreateTempDir()
{
  if (should_delete_ && p_.exists())
  {
    RemoveDirRecursive(p_.string());
  }
}

std::string jhmr::FindExeOnSystemPath(const std::string& exe_name)
{
  std::string exe_full_path;
  
  char* path_c_str = std::getenv("PATH");

  if (path_c_str)
  {
    const auto path_dirs = StringSplit(path_c_str, ":");

    for (const auto& path_dir : path_dirs)
    {
      Path p = path_dir;
      p += exe_name;

      if (p.exists() && p.is_exec())
      {
        exe_full_path = p.string();
        break;
      }
    }
  }
  else
  {
    jhmrThrow("could not access PATH variable!");
  }

  return exe_full_path;
}

std::string jhmr::FileToString(const std::string& path)
{
  FileInputStream fin(path);  

  std::string s;
  s.resize(fin.num_bytes_left());
  
  fin.read_remaining_bytes(&s[0]);

  return s;
}

namespace
{

template <class tScalar>
std::vector<tScalar> FileToScalarsHelper(const std::string& path)
{
  return jhmr::StringCast<tScalar>(jhmr::StringSplit(jhmr::FileToString(path), " \t\r\n"));
}

}  // un-named

std::vector<float> jhmr::FileToFloats(const std::string& path)
{
  return FileToScalarsHelper<float>(path);
}

std::vector<double> jhmr::FileToDoubles(const std::string& path)
{
  return FileToScalarsHelper<double>(path);
}

jhmr::CoordScalarList jhmr::FileToCoordScalars(const std::string& path)
{
  return FileToScalarsHelper<CoordScalar>(path);
}
