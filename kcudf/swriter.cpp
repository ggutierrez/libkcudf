/* -*- mode: C++; c-basic-offset: 2; indent-tabs-mode: nil -*- */
/*
 *  Main authors:
 *     Yves Jaradin <yves.jaradin@uclouvain.be>
 *     Gustavo Gutierrez <gutierrez.gustavo@uclouvain.be>
 *
 *  Copyright:
 *     Yves Jaradin, 2010
 *     Gustavo Gutierrez, 2010
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

#include <cassert>
#include <kcudf/swriter.hh>

using namespace std;

/*
 * KCudfFileWriter
 */

KCudfFileWriter::KCudfFileWriter(const char* fname)
  : KCudfWriter(), os(fname) {}

KCudfFileWriter::~KCudfFileWriter(void) {
  os.close();
}

void KCudfFileWriter::package(unsigned int id, bool keep, bool install, const char* desc) {
#ifndef NDEBUG
  cons.insert(cons.end(),id);
#endif
  os << "P " << id << " "
      << (keep ? "K" : "k") << " "
      << (install ? "I" : "i") << " # " << desc << std::endl;
}

void KCudfFileWriter::dependency(unsigned int id, unsigned int id2, const char* desc) {
#ifndef NDEBUG
  assert(cons.count(id) > 0);
  assert(cons.count(id2) > 0);
#endif
  if (id != id2)
    os << "D " << id << " " << id2 << " # " << desc << std::endl;
}

void KCudfFileWriter::conflict(unsigned int id, unsigned int id2, const char* desc) {
#ifndef NDEBUG
  assert(cons.count(id) > 0);
  assert(cons.count(id2) > 0);
#endif
  os << "C " << id << " " << id2 << " # " << desc << std::endl;
}

void KCudfFileWriter::provides(unsigned int id, unsigned int id2, const char* desc) {
#ifndef NDEBUG
  assert(cons.count(id) > 0);
  assert(cons.count(id2) > 0);
#endif
  os << "R " << id << " " << id2 << " # " << desc << std::endl;
}

/*
 * KCudfInfoFileWriter
 */
KCudfInfoFileWriter::KCudfInfoFileWriter(const char* fname)
  : KCudfInfoWriter(), os(fname) {}


KCudfInfoFileWriter::~KCudfInfoFileWriter(void) {
  os.close();
}

void
KCudfInfoFileWriter::package(unsigned int id, unsigned int version, const char* name) {
  os << id << " " << version << " " << name << std::endl;
}

/*
 * KCudfMemWriter
 */

KCudfMemWriter::KCudfMemWriter(void) : KCudfWriter() {}

KCudfMemWriter::~KCudfMemWriter(void) {}

void KCudfMemWriter::package(unsigned int id, bool keep, bool install, const char*) {
  packages.insert(id);
  if (keep) keeps.insert(id);
  if (install) installs.insert(id);
}

void KCudfMemWriter::dependency(unsigned int id, unsigned int id2, const char*) {
  deps[id].insert(id2);
}

void KCudfMemWriter::conflict(unsigned int id, unsigned int id2, const char*) {
  confs[id].insert(id2);
  confs[id2].insert(id);
}

void KCudfMemWriter::provides(unsigned int id, unsigned int id2, const char*) {
  pvds[id].insert(id2);
}

/*
 * KCudfInfoMemWriter
 */
KCudfInfoMemWriter::KCudfInfoMemWriter(void) : KCudfInfoWriter() {}

KCudfInfoMemWriter::~KCudfInfoMemWriter(void) {}

void KCudfInfoMemWriter::package(unsigned int id, unsigned int version, const char* name) {
  info.insert(info.end(),make_pair(id,make_tuple(version,std::string(name))));
}

/*
 * Translate
 */
void translate(CudfDoc& doc, const char* kcudf, const char* info) {
  KCudfFileWriter kcudf_wrt(kcudf);
  KCudfInfoFileWriter inf_wrt(info);

  KCudfTranslator ts(doc);
  ts.translate(kcudf_wrt, inf_wrt);
}
