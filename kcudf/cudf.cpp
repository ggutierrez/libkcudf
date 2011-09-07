/* -*- mode: C++; c-basic-offset: 2; indent-tabs-mode: nil -*- */
/*
 *  Main authors:
 *     Yves Jaradin <yves.jaradin@uclouvain.be>
 *     Gustavo Gutierrez <gutierrez.gustavo@uclouvain.be>
 *
 *  Copyright:
 *     Yves Jaradin, 2009
 *     Gustavo Gutierrez, 2009
 *
 *  Last modified:
 *     $Date$ by $Author$
 *     $Revision$
 *
 *
 *  Permission is hereby granted, free of charge, to any person obtaining
 *  a copy of this software and associated documentation files (the
 *  "Software"), to deal in the Software without restriction, including
 *  without limitation the rights to use, copy, modify, merge, publish,
 *  distribute, sublicense, and/or sell copies of the Software, and to
 *  permit persons to whom the Software is furnished to do so, subject to
 *  the following conditions:
 *
 *  The above copyright notice and this permission notice shall be
 *  included in all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 *  LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 *  OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 *  WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include <assert.h>
#include <kcudf/cudf.hh>

using namespace std;

Property::Property(void) : prop() {}

Property::Property(const char* p) : prop(p) {}

std::string Property::getVal(void) const {
  return prop;
}

const Property Property::empty;

PkUnit::PkUnit(const std::string& n, int v)
: name_(n), version_(v) {}

PkUnit::PkUnit(const CudfPackage& pk)
: name_(pk.name()), version_(pk.version()) {}

PkUnit::PkUnit(const PkUnit& pk)
: name_(pk.name_), version_(pk.version_) {}

const std::string& PkUnit::getName(void) const {
  return name_;
}

int PkUnit::version(void) const {
  return version_;
}

void PkUnit::version(int v) {
  version_ = v;
}

bool PkUnit::operator&&(const Vpkg& vp) const {
  // both have the same name
  if (name_.compare(vp.getName()) != 0)
    return false;
  switch (vp.getRel()) {
    case ROP_EQ:
      return version_ == (int)vp.getVersion();
    case ROP_NEQ:
      return version_ != (int)vp.getVersion();
    case ROP_LE:
      return version_ <= (int)vp.getVersion();
    case ROP_LT:
      return version_ < (int)vp.getVersion();
    case ROP_GE:
      return version_ >= (int)vp.getVersion();
    case ROP_GT:
      return version_ > (int)vp.getVersion();
    case ROP_NOP:
      return true;
  }
  assert(false);
  return false;
}

