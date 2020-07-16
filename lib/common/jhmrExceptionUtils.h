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

#ifndef JHMREXCEPTIONUTILS_H_
#define JHMREXCEPTIONUTILS_H_

#include <cstring>
#include <cstdarg>
#include <cstdio>
#include <exception>

namespace jhmr
{

class StringMessageException : public std::exception
{
public:

  // This was originally a template parameter, but we never used different
  // sizes, so hard-coding now.
  enum { kBUF_CAPACITY = 512 };

  StringMessageException();

  explicit StringMessageException(const char* msg_fmt, ...);

  virtual ~StringMessageException() throw() { }

  virtual const char* what() const throw()
  {
    return buf_;
  }

  void set_msg(const char* msg_fmt, ...);

protected:
  void set_msg(const char* msg_fmt, va_list& ap);

private:

  char buf_[kBUF_CAPACITY];

};

}  // jhmr

#define jhmrDeriveStringMessageException(DERIVED_NAME) \
  class DERIVED_NAME : public jhmr::StringMessageException \
  { \
  public: \
    typedef jhmr::StringMessageException parent_type; \
    \
    DERIVED_NAME() : parent_type() \
    { } \
    \
    explicit DERIVED_NAME(const char* msg_fmt, ...) : parent_type() \
    { \
      va_list ap; \
      va_start(ap, msg_fmt); \
      set_msg(msg_fmt, ap); \
      va_end(ap); \
    } \
    \
    virtual ~DERIVED_NAME() throw() { } \
  };

namespace jhmr
{

using LazyMessageException = StringMessageException;

}  // jhmr

// Defining like this allows for variadic arguments. e.g. jhmrThrow("Failed! %d", err_code);
#define jhmrThrow throw jhmr::LazyMessageException

#endif
