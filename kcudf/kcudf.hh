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

#ifndef __KCUDF__HH__
#define __KCUDF__HH__

#include <iostream>
#include <stdexcept>
#include <fstream>
#include <vector>
#include <set>
#include <map>

#include <kcudf/cudf.hh>


#include <list> // TODO: should be replaced by a vector!

/**
 * \brief Represents a package inside the translator.
 *
 */
class Package {
private:
  /// The package is installed or will be installed by the solver
  bool install;
  /// The install property will be kept or not
  bool keep;
  /// Package identifier
  unsigned int id;
  /// Package version
  int version;
  /// Conflicts associated to the package
  std::set<unsigned int> conflicts;
  /// Dependencies associated to the package
  std::set<unsigned int> dependencies;
  /// Functionality provided by this package
  std::set<unsigned int> provides;
  /// Used to create a consecutive id for each package
  static unsigned int next_id;
protected:
  /// Information about the package
  std::string info;
  /// Information about the keep operations
  std::string keep_info;
public:
  /// Constructor
  Package(bool inst, int v);
  /// Destructor
  virtual ~Package(void);
  /// Adds \a p as a conflict to this package
  virtual void addConflict(unsigned int p);
  /// Adds \a p as a dependency of this package
  virtual void addDependency(unsigned int p);
  /// Return the identifier
  virtual unsigned int getId(void) const;
  /// Return the version
  virtual int getVersion(void) const;
  /// Return if the package represented is concrete or not
  virtual bool isConcrete(void) const = 0;
  /// Mark a package as install / uninstall
  virtual void markInstall(bool st);
  /// Tests if the package is marked or not as install
  virtual bool markedInstall(void) const;
  /// Mark a package as keep or not
  virtual void markKeep(bool st);
  /// Tests if the package is marked or not as keep
  virtual bool markedKeep(void) const;
  /// Output package information to stream \a o
  void toStream(std::ostream& o, const std::map<unsigned int, Package*>& packages) const;
  /// Return the dependencies
  const std::set<unsigned int>& getDependencies(void) const;
  /// Return the conflicts
  const std::set<unsigned int>& getConflicts(void) const;
  /// Return the information about the package
  const char* getInfo(void) const;
  /// Return the information about the package
  const char* getKeepInfo(void) const;
  /// Add info to the package
  void addInfo(const char* ninf);
  /// Add keep info to the package
  void addKeepInfo(const char* ninf);
};

/// Output package to \a o
std::ostream& operator<< (std::ostream& o,const Package& p);

/**
 * \brief Represents a disjunction in a \a Cudf document.
 *
 * Disjunctions are present in an explicit way in the dependency statement of a cudf
 * document; and, in a non explicit way when a package is provided by several
 * concrete packages.
 *
 * As part of the translation process, some disjunctions semantically represent the
 * same so there is some support in this class to forward disjuntions.
 */
class Disjunction : public Package {
private:
  /// Indicates if the disjunction is forwarded to another
  bool forwarded;
  /// Equivalent package representing this
  Package *fwd;
  /// Providers for the represented disjunction
  std::set<unsigned int> providers;
  /// Package that has to be removed from the interpretation of the providers
  unsigned int conf_but;
  /// Indicates if the set of providers should be interpreted without this package
  bool has_but;
  /// Indicates if the disjunction is already flatten
  bool flt;
  using Package::info;
  /// Default constructor
  Disjunction(void);
public:
  Disjunction(const char* info = NULL);
  Disjunction(int v, const char* info = NULL);
  virtual ~Disjunction();
  /// Return the id associated with the disjunction
  virtual unsigned int getId(void) const;
  /// Return the version
  virtual int getVersion(void) const;
  /// Adds \a p to the set of conflicts of the disjunction
  virtual void addConflict(unsigned int p);
  /// Adds \a p to the set of dependencies
  virtual void addDependency(unsigned int p);
  /// Adds \a p as a provider of the disjunction
  void addProvider(unsigned int p);
  /// Returns the set of providers for this disjunction
  std::set<unsigned int>& getProviders(void);
  /// Returns the set of providers for this disjunction
  const std::set<unsigned int>& getProviders(void) const;
  /// Adds \a p as an exception of the conflicts
  void addBut(unsigned int p);
  /// returns the but: only valid if hasBut returns true.
  unsigned int but(void) const;
  /// Test whether this contains a but or not.
  bool hasBut(void) const;
  /// Return if the package represented is concrete or not
  bool isConcrete(void) const;
  /// Mark the package as flatten
  void flat(std::map<unsigned int,Package*>& pkgs);
  /// Test whether the package is already flatten
  bool isFlat(void) const;
  /// Set the current disjunction to package p
  void setForward(Package *p);
  /// Mark a package as install or not
  virtual void markInstall(bool st);
  /// Tests if the package is marked or not as install
  virtual bool markedInstall(void) const;
  /// Mark a package as keep or not
  virtual void markKeep(bool st);
  /// Tests if the package is marked or not as install
  virtual bool markedKeep(void) const;

};

/**
 * \brief Represents a concrete package in a \a Cudf document.
 */
class SelfPackage : public Package {
  /// Name of the package
  std::string nm;
  using Package::info;
public:
  /// Creates a concrete package with installation status \a inst and version \a v
  SelfPackage(const std::string& name, bool inst, int v);
  virtual ~SelfPackage(void);
  /// Return if the package represented is concrete or not
  bool isConcrete(void) const;
  /// Return the name of the package
  const std::string& name(void) const;
};

/**
 * \brief Translation statistics
 */
class TranslatorStats {
public:
  /// Concrete packages
  unsigned int cp;
  /// Real disjunctions
  unsigned int rd;
  /// Equal disjunctions
  unsigned int ed;
  /// Zero provided disjunctions
  unsigned int zp;
  /// Fail detected
  bool fail;
  /// Constructor
  TranslatorStats(void);
};

class DTNode;
class KCudfWriter;
class KCudfInfoWriter;

class KCudfData {
  friend std::ostream& operator<< (std::ostream& o,const KCudfData& kcudf);
private:
  /// Maps package identifiers to packages
  std::map<unsigned int,Package*> packages;
  /// Maps identifiers to concrete packages
  std::map<std::string,std::map<int,int> > concrete;
  /// Maps package name (string) to a map of package version to package id.
  std::map<std::string,std::map<int,int> > specv;
  /// Maps version constraint specs to package id.
  std::map<std::string,int> constv;
  /**
   * \brief Process and load all the information present in the cudf
   * document regarding concrete packages.
   */
  void processConcretePackages(const CudfDoc& doc);
  /**
   * \brief Process the information about installed packages.
   *
   * This is to support paranoid optimization criteria.
   */
  void processInstalledPackages(const CudfDoc& doc);
  /**
   * \brief Process and load all the information regarding equality constraints.
   *
   * This information will be used further to solve the version
   * constraints present in dependencies, conflicts and requests.
   */
  void processEqualityConstraints(const CudfDoc& doc);
  /**
   * \brief Process the provide statement of every package by looking
   * only for unconstrained provides.
   *
   * Provides in the way "provides: something = v" were already
   * handled in the \a processEqualityConstraints method and no ohter
   * way of provide statements is allowed.
   */
  void processProvides(const CudfDoc& doc);
  /**
   * \brief Process and load all the information regarding range
   * constraints and disjunctions.
   */
  void processRangeConstraints(const CudfDoc& doc);
  /**
   * \brief Process request
   *
   * After reading all the information contained in the package
   * universe and in the request, this method is in charge of encoding
   * the request by marking the corresponding packages as install or
   * keep.
   */
  void processRequest(const CudfDoc& doc, DTNode* dsj);
  /**
   * \brief Add a new disjunction for \a name and \a version if it
   * does not exist. The return value will depend on the existence or
   * not of the disjunction
   */
  Package* addDisjunction(const std::string& name, unsigned int version);
  Package* addDepDisjunction(const std::string& name, unsigned int version);
  /**
   * \brief Solve the version package constraint \a c according to the
   * information in \a specv and in \a constv and append the packages
   * satisfying it to pkgs.
   */
  void solveConstraint(const Vpkg& c, std::vector<unsigned int>& pkgs) const;
  /**
   * \brief Creates a disjunction package and stores it under string
   * \a s in \a constv.
   */
  Disjunction* newDisjunction(const std::string& s);
  /**
   * \brief Returns a new disjunction expressing the constraint in \a
   * cs. If a disjunction already exist under \a name then it is
   * returned.
   */
  Disjunction* getDisjunction(const Vpkg& cs);
  Disjunction* getDepDisjunction(const Vpkg& cs);
  /**
   * \brief Returns a new disjunction with an empty set of providers
   * (if no disjuntion exist under \a name) or returns an existent
   * disjunction.
   */
  Disjunction* getDisjunction(const std::string& name);
  /**
   * \brief Creates pairwise conflict between the packages in \a s.
   */
  void pairwiseConflicting(const std::set<unsigned int>& s);
  /**
   * \brief Fix install attributes for virtual packages
   */
  void fixInstallVirtuals(void);
  /**
   * \brief Return the number of installed providers for the disjunction
   *\a d.
   *
   * This method can only be called after fixing all the virtuals
   */
  unsigned int installedProviders(unsigned int d) const;
  bool consistent(void) const;
  void readLine(const std::string& s, std::vector<std::string>& d);
  void readPackage(const std::string& s);
private:
  /**
   * \brief Data structure to store big packages.
   *
   * Big packages are just virtual packages that are provided by the concrete
   * packages with the same name. For example, if during the parser we find <p,1>,
   * <p,5> and <p,10> and at least one of them is installed then a virtual package
   * is created <p-any> and is provided by the parsed packages. This is the storage
   * data structure for all the <p-any>'s found in the document.
   */
  std::set<int> bigPackages_;
  /// List of installed concrete packages
  std::vector<int> crtPackages_;
  /// List of _consistent_ installed concrete packages
  mutable std::vector<int> conPackages_;
public:
  /// Constructor from a cudf document
  KCudfData(const CudfDoc& doc, TranslatorStats& stats);
  /// Constructor from a kcudf file
  KCudfData(const char* fname);
  /// Return all the information about packages
  const std::map<unsigned int,Package*>& getPackages(void) const;
  /// Return the set of installed big packages
  const std::set<int>& bigPackages(void) const;
  /// Return the list of installed concrete packages
  const std::vector<int>& crtPackages(void) const;
};

/// Output the information stored in \a kcudf
std::ostream& operator<< (std::ostream& o,const KCudfData& kcudf);

/**
 * \brief Tree representing the disjunctions.
 *
 * The idea behind this class is to build an nary tree structure to
 * store already computed disjunctions and then to avoid repeating
 * nodes (and edges) in the providers graph if the same disjunction is
 * present for several packages.
 */
class DTNode {
private:
  /**
   * \brief Id of the node in the graph representing the disjunction,
   * meaningful only if \a cmp is true.
   */
  unsigned int tree_node;
  /// Represents if the node has been computed or not
  bool cmp;
  /// Map storing the children of the node.
  std::map<unsigned int,DTNode*> children;
public:
  /// Constructor
  DTNode(void);
  /// Destructor
  ~DTNode(void);
  /**
   * \brief Adds a disjunction to the tree and returns the id
   * representing it. If the disjunction is already present then the
   * identifier representing it is returned, if not it is created.
   */
  unsigned int addDisjunction(unsigned int id, const std::set<unsigned int>& pvds);
  /**
   * \brief Test if the node is computed or not.
   *
   * If the node is already computed means that there is a disjunction
   * representing this node.
   */
  bool computed(void) const;
  /// Associates this node of the tree to the node \a n in the graph.
  void setNode(unsigned int n);
  /// Returns the node in the graph associated with the disjunction
  unsigned int getNode(void) const;
  /// Test if there is a child for \a u.
  bool hasChild(unsigned int u) const;
  /// Returns the child associated to \a u. Only valid if hasChlid(u)
  DTNode* getChild(unsigned int u);
  /// Adds a child \a c to the tree under the key \a u
  void addChild(unsigned int u, DTNode* c);
};

/**
 * \brief General KCudf exception
 */
class KCudfFailure : public std::runtime_error {
public:
  KCudfFailure(const char* s) : std::runtime_error(s) { }
};

/**
 * \brief Exception when it is not possible to fulfill the request.
 *
 * This exception is thrown when from the kernel cudf translation is
 * possible to detect that the request cannot be satisfied.
 */
class KCudfFailedRequest : public KCudfFailure {
public:
  KCudfFailedRequest(const char* s) : KCudfFailure(s) { }
};

/**
 * \brief Exception for malformed provide descriptions
 *
 * This exception is thrown during parsing the cudf file and an
 * incorrect provide statement is found.
 */
class KCudfInvalidProvide : public KCudfFailure {
public:
  KCudfInvalidProvide(const char* s) : KCudfFailure(s) { }
};
/**
 * \brief Writer for kcudf parsed documents.
 *
 * A writer of kcudf documents defines the way to treat parsed kcudf information. By
 * default an instance of this class does nothing. It has to be defined by inheritance
 * to do something useful with the information.
 */
class KCudfWriter {
public:
  /// Constructor
  KCudfWriter(void);
  /// Destructor
  virtual ~KCudfWriter(void);
  /**
   * \brief Process package with identifier \a p and read information about
   * the current state (\a install) and the constraints about the availability
   * of the package: \a keep.
   */
  virtual void package(unsigned int p, bool keep, bool install,
                       const char* desc);
  /**
   * \brief Process dependency relation: \a p depends on \a q.
   */
  virtual void dependency(unsigned int p, unsigned int q, const char* desc);
  /**
   * \brief Process conflict relation: \a p conflicts with \a q.
   */
  virtual void conflict(unsigned int p, unsigned int q, const char* desc);
  /**
   * \brief Process provides relation: \a p provides \a q.
   */
  virtual void provides(unsigned int p, unsigned int q, const char* desc);
};

class KCudfInfoWriter {
public:
  KCudfInfoWriter(void);
  virtual ~KCudfInfoWriter(void);
  /**
   * \brief This method is called for every concrete package existing in the
   * kcudf input.
   */
  virtual void package(unsigned int id, unsigned int version, const char* name);
};

/**
 * \brief Class to handle the translation of a \a Cudf document into \a Kcudf.
 */
class KCudfTranslator {
private:
  /// Reference to the cudf document
  const CudfDoc& doc;
  /// Statistics of the translation process
  TranslatorStats st;
  /// Interpretation of the Cudf
  KCudfData data;
  /// Default constructor
  KCudfTranslator();
  /// Helper method to write information about packages
  void writePackages(KCudfWriter& wrt, KCudfInfoWriter& inf, bool debug);
  /// Helper method to write information about concrete packages
  void writeConcreteSelfProvided(KCudfWriter& wrt, bool debug);
  /// Helper method to write information about dependencies
  void writeDependencies(KCudfWriter& wrt, bool debug);
  /// Helper method to write information about conflicts
  void writeConflicts(KCudfWriter& wrt, bool debug);
  /// Helper method to write information about provides
  void writeProvides(KCudfWriter& wrt, bool debug);
public:
  /// Constructor
  KCudfTranslator(const CudfDoc& d);
  /**
   * \brief Translate the document.
   *
   * To keep the output clean, the order of writing the file is:
   * - Packages
   * - Dependencies
   * - Conflicts
   * - Provides
   */
  void translate(KCudfWriter& w, KCudfInfoWriter& i, bool dbg = false);
  /// Return translation statistics
  const TranslatorStats& stats(void) const;
  /**
   * \brief Return additional information needed for the paranoid track.
   *
   * \a search are the packages that must become part of the search because at
   * least one concrete package with the same name was found installed.
   *
   * \a crt contains the installed concrete packages that were found
   */
  void extraParanoid(std::vector<int>& search) const;
  /// Write paranoid information on stream \a os
  void writeParanoid(std::ostream& big) const;
  /// Return the set of installed big packages
  const std::set<int>& bigInstalled(void) const;
  /// Return the list of installed concrete packages
  const std::vector<int>& crtInstalled(void) const;
};

/**
 * \brief Failed stream exception
 */
class FailedStream : public std::runtime_error {
public:
  FailedStream(const char* s) : std::runtime_error(s) { }
};

/**
 * \brief Parse the kcudf in \a input and handle all the information contained in it
 * through the writer \a wrt.
 *
 * It is up to the writer to decide how to handle this information. Some writers
 * are provided to output a dot representation of the read information (see
 * \a KCudfDotWriter)
 *
 * \warning This procedure will generate self dependencies on the fly
 * (i.e. even if they are not explicitly stated in \a fname) on all packages
 * and will pass them to the writer; it is up to the writer to take or ignore
 * them.
 *
 * \warning As kcudf does not make any explicit distinction between virtual and
 * concrete packages, a provided relation (on the concrete ones) is added during
 * the translation process.
 *
 * \warning This procedure does not care abut any repeated relation so it is up
 * to the writer to implement this functionality if required.
 */
void read(std::istream& input, KCudfWriter& wrt);
/**
 * \brief Exception for malformed KCUDF file
 *
 * This exception is thrown during parsing the kcudf file and an
 * unknown statement is found.
 */
class KCudfReaderInvalidStatement : public KCudfFailure {
public:
  KCudfReaderInvalidStatement(const char* s) : KCudfFailure(s) { }
};
/**
 * \brief Reads the info file \a info and puts the information in \a m.
 */
void readInfo(const char* info, std::map<std::string,std::map<unsigned int, unsigned int> >& m);
void readInfo(const char* info, KCudfInfoWriter& wrt);

/**
 * \brief Convenience function to translate the cudf represented in \a doc and
 * output the resulting data in \a kcudf and \a info files. The return value
 * contains the big packages for which at least one package version was found
 * installed.
 */
void
translate(const CudfDoc& doc, const char* kcudf, const char* info,
          std::vector<int>& bigInstalled, std::vector<int>& crtInstalled,
          const char* paranoid = NULL);

/**
 * \brief Updates \a doc with the information contained in \a kcudf0 and \a kcudf1
 * the information used for the translation is in \a info.
 */
void update(CudfDoc& doc, const char* info, const char* kcudf0, const char* kcudf1);

/**
 * \brief Writer for updating a cudf document
 */
class CudfUpdater : public KCudfWriter {
private:
  /// Mapping kcudf identifiers to package pointers
  std::map<unsigned int, CudfPackage*> status;
  CudfUpdater(void);
  /// Number of packages that were updated
  unsigned int changed;
public:
  CudfUpdater(CudfDoc& doc, std::map<std::string,std::map<unsigned int, unsigned int> >& m);
  void package(unsigned int id, bool keep, bool install, const char* desc);
  void dependency(unsigned int id, unsigned int id2, const char* desc);
  void conflict(unsigned int id, unsigned int id2, const char* desc);
  void provides(unsigned int id, unsigned int id2, const char* desc);
  unsigned int stats(void) const;
};
/**
 * \brief Convenience function to update \a doc with the information contained
 * in files \a info, \a easy and \a solved.
 *
 * \warning Only solved can be a NULL representing the fact that everything was
 * probably solved by the translator or the reducer.
 *
 * \warning TODO: exception for files
 * \callgraph
 */
void format(CudfDoc& doc, const char* info, const char* easy,
            const char* solved = NULL);
void format(CudfDoc& doc, const char* info, const char* easy,
            std::istream& solved);

class KCudfInfoMapWriter : public KCudfInfoWriter {
public:
  typedef std::map<unsigned int,std::string> id_to_name_t;
  typedef std::map<unsigned int,unsigned int> id_to_version_t;
private:
  /// Data structure to store the contents of the info file (maps id's to names)
  id_to_name_t names;
  id_to_version_t versions;

public:
  /// Constructor
  KCudfInfoMapWriter(void);
  virtual ~KCudfInfoMapWriter(void);
  void package(unsigned int id, unsigned int version, const char* name);
  const KCudfInfoMapWriter::id_to_name_t& getNames(void) const;
  const KCudfInfoMapWriter::id_to_version_t& getVersions(void) const;
};

#endif
