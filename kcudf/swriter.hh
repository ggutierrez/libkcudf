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

#ifndef __KCUDF__SWRITER__HH__
#define __KCUDF__SWRITER__HH__

#include <kcudf/kcudf.hh>

/**
 * \file This file contains the implementations of standard kcudf writers.
 *
 * These writers are intended for the general purpose: dumping a kcudf to a file is
 * done by \a KCudfFileWriter and abstracting it in memory is done by
 * \a KCudfMemWriter
 */

/**
 * \brief Dumps KCUDF information to a file.
 *
 * \warning If compiled in debug mode, a sanity check for every realtion is done.
 * it is quite trivial but useful: after reading a dependency, conflict or provide
 * relation, both identifiers are checked as already existent packages.
 */
class KCudfFileWriter : public KCudfWriter {
private:
  std::ofstream os;
#ifndef NDEBUG
  // consistency check data structure
  std::set<unsigned int> cons;
#endif
protected:
  /// Default constructor
  KCudfFileWriter(void);
public:
  /// Constructor for using \a fname as output
  KCudfFileWriter(const char* fname);
  /// Destructor
  virtual ~KCudfFileWriter(void);
  /**
   * \brief Writes package information to the output file.
   *
   * \warn It is possible to state that every package depends on itself. This fact
   * can be used by any solver or can be ignored. For the size of the output file
   * this relation is omited, However the reader will report it in any case.
   */
  virtual void package(unsigned int id, bool keep, bool install, const char* desc);
  /// Writes dependency information to the output file
  virtual void dependency(unsigned int id, unsigned int id2, const char* desc);
  /// Writes conflict information to the output file
  virtual void conflict(unsigned int id, unsigned int id2, const char* desc);
  /// Writes disjunction information to the output file
  virtual void provides(unsigned int id, unsigned int id2, const char* desc);
};

/**
 * \brief Info wrtier to write KCudf informaion to a file
 */
class KCudfInfoFileWriter : public KCudfInfoWriter {
private:
  std::ofstream os;
  /// Default constructor
  KCudfInfoFileWriter(void);
public:
  /// Constructor for using stream  \a o for output
  KCudfInfoFileWriter(const char* fname);
  /// Destructor
  virtual ~KCudfInfoFileWriter(void);
  /// Write information about package \a id
  virtual void package(unsigned int id, unsigned int version, const char* name);
};

/**
 * \brief Writer that stores the content of a KCudf file into memory. The data
 * structures used to store the relations are sets and maps.
 *
 * \warning No extra information about packages nor relations is stored
 */
class KCudfMemWriter : public KCudfWriter {
protected:
  /// Packages storage
  std::set<unsigned int> packages;
  /// Packages marked as keep
  std::set<unsigned int> keeps;
  /// Packages marked as install
  std::set<unsigned int> installs;
  /// Dependencies storage
  std::map<unsigned int, std::set<unsigned int> > deps;
  /// Conflict storage
  std::map<unsigned int, std::set<unsigned int> > confs;
  /// Provides storage
  std::map<unsigned int, std::set<unsigned int> > pvds;
public:
  /// Default constructor
  KCudfMemWriter(void);
  /// Destructor
  virtual ~KCudfMemWriter(void);
  /// Register a package
  virtual void package(unsigned int id, bool keep, bool install, const char*);
  /// Register a depependency
  virtual void dependency(unsigned int id, unsigned int id2, const char*);
  /// Register a conflict
  virtual void conflict(unsigned int id, unsigned int id2, const char*);
  /// Register a provides
  virtual void provides(unsigned int id, unsigned int id2, const char*);
};

/**
 * \brief Info wrtier to write KCudf informaion to a map in memory
 */
class KCudfInfoMemWriter : public KCudfInfoWriter {
private:
  /// Map to store the package information
  std::map<unsigned int,std::tuple<unsigned int, std::string> > info;
public:
  /// Constructor for using stream  \a o for output
  KCudfInfoMemWriter(void);
  /// Destructor
  virtual ~KCudfInfoMemWriter(void);
  /// Write information about package \a id
  virtual void package(unsigned int id, unsigned int version, const char* name);
};

/**
 * \brief Translates \a doc into kcudf and writes the results to  \a kcudf (the
 * corresponding kcudf translation) and \a info (the corresponing info mapping)
 */
void translate(CudfDoc& doc, const char* kcudf, const char* info);

#endif
