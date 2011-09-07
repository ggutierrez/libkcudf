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

#ifndef __KCUDF_CUDF_HPP__
#define __KCUDF_CUDF_HPP__

#include <cudf.h>



class Property {
private:
  std::string prop;
public:
  Property(void);
  Property(const char* p);
  std::string getVal(void) const;
  static const Property empty;
};


class PkUnitComp;

/**
 * \brief Represent a package unit, this is, an specific version of a
 * package.
 *
 * Objects of this class are used, for example, to specify by
 * extension the set of available packages that satisfy certain
 * constraint.
 */
class PkUnit {
public:
  friend class PkUnitComp;
  friend std::ostream& operator<< (std::ostream& o, const PkUnit& pu);
private:
  /// Package name
  std::string name_;
  /// Package version
  int version_;
public:
  /// \name Constructors
  //@{
  /// Constructor from a cudf package \a pk
  PkUnit(const CudfPackage& pk);
  /// Constructor from a string \n representing the name and version \a v
  PkUnit(const std::string& n, int v);
  /// Copy constructor
  PkUnit(const PkUnit& pu);
  //@}
  /// \name Access
  //@{
  /// Returns the name of the unit
  const std::string& getName(void) const;
  /// Returns the version of the unit
  int version(void) const;
  //@}
  /// \name Modifiers
  //@{
  void version(int v);
  //@}
  /// \name Compatibility test
  //@{
  /**
   * \brief Tests whether the unit is compatible with the versioned
   * package \a vp.
   *
   * A package unit is said to be compatible with a versioned package
   * iff:
   *
   * - The name of the pkunit is the same as the name of the vp, and
   * - \a version R_{rel(vp)} version(vp) holds
   *
   * \warn If the versioned package does not have a relation operator
   * defined (i.e. ROP_NOP) then a pacakge unit is compatible with it
   * by having the same name.
   */
  bool operator&&(const Vpkg& vp) const;
  //@}
};

/**
 * \brief Functor to order collections of package units.
 *
 * Given two package units \a lhs and \a rhs, \f$ lhs < rhs \f$ iff:
 *
 * - name(lhs) <_{lex} name(rhs), or
 * - name(lhs) = name(rhs) and version(lhs) < version(rhs)
 */
struct PkUnitComp {
  bool operator() (const PkUnit& lhs, const PkUnit& rhs) const {
    if (lhs.name_ < rhs.name_) return true;
    else if (lhs.name_ > rhs.name_) return false;
    return lhs.version_ < rhs.version_;
  }
};

#endif
