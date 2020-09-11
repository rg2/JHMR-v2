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

#include "jhmrVersionInfo.h"

#include <fmt/format.h>

extern const char* jhmrGIT_SHA1;

extern bool jhmrHAS_GIT_SHA1;

extern const char* jhmrPROJ_VERSION;

std::string jhmr::ProjVerStr()
{
  return jhmrPROJ_VERSION;
}

bool jhmr::HasGitSHA1()
{
  return jhmrHAS_GIT_SHA1;
}

std::string jhmr::GitSHA1()
{
  return jhmrGIT_SHA1;
}

std::string jhmr::ProjVerStrAndGitSHA1IfAvail()
{
  if (HasGitSHA1())
  {
    return fmt::format("{} ({})", ProjVerStr(), GitSHA1());
  }
  else
  {
    return ProjVerStr();
  }
}

