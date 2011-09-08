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

#ifndef __KCUDF__REDUCE__HH__
#define __KCUDF__REDUCE__HH__

#include <kcudf/kcudf.hh>
#include <kcudf/gwriter.hh>

/**
 * \brief Possible states for a package inside the reducer.
 */
enum PKR_STATE {
  PKR_CU = 0, ///> The package can be uninstalled
  PKR_CI,     ///> The package can be installed
  PKR_MU,     ///> The package must be uninstalled
  PKR_MI,     ///> The package must be installed
  PKR_SR,     ///> The state of the package will be determined by the solver
  PKR_FL,     ///> Failure state
  PKR_AB      ///> Abort state
};

/**
 * \brief Possible operations on a package during the reducing process.
 */
enum PK_OP {
  PK_MU = 0, ///> Must uninstall
  PK_MI,     ///> Must install
  PK_CI,     ///> Can install
  PK_CU,     ///> Can uninstall
  PK_UCP,    ///> Update candidate providers
  PK_USP,    ///> Update safe providers
  PK_UPD     ///> Update packages
};

#ifndef NDEBUG
/// Output of package status
std::ostream& operator<< (std::ostream& o,const PKR_STATE st);
/// Output of package operations
std::ostream& operator<< (std::ostream& o,const PK_OP op);
#endif
/**
 * \brief Store information about reduction statistics.
 *
 * Several statistics ar collected by this class, all of them in terms of number of
 * packages or number of relations among them.
 */
class ReducerStats {
public:
  /// Number of initial packages
  unsigned int pkgs;
  /// Number of packages ending in search state
  unsigned int pkg_srch;
  /// Number of other packages that need to be considered
  unsigned int pkg_is;
  /// Number of already solved packages
  unsigned int pkg_slvd;
  /// Number of packages ending in CU or MU
  unsigned int pkg_nis;
  /// Number of dependencies interesting to the solver
  unsigned int deps;
  /// Number of conflicts interesting to the solver
  unsigned int confs;
  /// Number of provides interesting to the solver
  unsigned int pvds;
  /// A solution was find by the reducer
  bool solution;
  /// A failure was find by the reducer
  bool fail;
  /// Failure state
  std::string failure;
  /// Constructor
  ReducerStats(void);
};

/// Output of reducer statistics to stream \a os.
std::ostream& operator <<(std::ostream& os, const ReducerStats& st);

/**
 * \brief Reducer for kcudf specifications.
 */
class KCudfReducer : public GraphWriter {
private:
  /// Transition function of the reducer
  static PKR_STATE tf[5][4];
  /// Update function
  void update(unsigned int pid);
  /// Type for tasks to be done
  typedef std::pair<PK_OP,unsigned int> task_t;
  /// First todo list
  std::list<task_t> todo1;
  /// Second todo list
  std::list<task_t> todo2;
  /// Enumeration to identify the end list of a task
  enum TD_LST {
    TD_1, /// Task for the todo list 1
    TD_2, /// Task for the todo list 2
  };
  /// Current state of each package
  std::map<unsigned int, PKR_STATE> pkg_st;
  /// Safe providers for each package
  std::map<unsigned int, unsigned int> sp;
  /// Candidate providers for each package
  std::map<unsigned int, unsigned int> cp;
  /// Statistics of the reduction process
  ReducerStats st;
  /// Returns the next task to do.
  task_t nextTask(void);
  /**
   * \brief Returns add a task on list \a td to perform operation \a
   * op on package \a pk
   */
  void addTask(PK_OP op, unsigned int pk, TD_LST td);
  /// Tests whehter there is work to do or not
  bool workTodo(void) const;
  /// TODO: documment!
  static bool isSP(PKR_STATE st);
  static bool isSPI(PKR_STATE st);
  static bool isCP(PKR_STATE st);
  /**
   * \brief Write the relations of package \a pkg using the writer \a wrt.
   * Only the elements in srch are taken into account.
   */
  void writeRelations(const Package& pkg, KCudfWriter& wrt,
                      const std::set<unsigned int>& srch) const;
  /**
   * \brief Output all the packages with their states.
   */
  void printPackages(std::ostream& os) const;
  /// Tests whether package \a p exists as a registered package
  bool exist(unsigned int p) const;
#ifndef NDEBUG
  /// Dump both todo lists on std::cerr
  void printWork(void) const;
#endif
public:
  /// Final state after a run of the reducer
  enum RD_OUT {
    RDO_FAIL,   /// The problem has no solution
    RDO_SOL,    /// The problem has a solution an the reducer found it
    RDO_SEARCH, /// The problem has been reduced and search is needed
  };
private:
  /**
   * \brief Apply the reducing process to the packges.
   *
   * The return value is either \a RDO_FAIL in the case that the
   * reducer is able to detect that there is no solution to the
   * problem and \a RDO_SEARCH in _any_ other case. Note that the \a
   * writeOutput method will return a more accurate result if the
   * problem can be solved by using only reduction.
   */
  RD_OUT process(void);
  /// Modify the state of package \a p to \a st
  void state(unsigned int p, PKR_STATE st);
  /// Write all the dependency relations of package \a pkg which are in search state
  void incDeps(unsigned int pkg, KCudfWriter& wrt);
  /// Write all the conflit relations of package \a pkg which are in search state
  void incConfs(unsigned int pkg, KCudfWriter& wrt);
  /// Write all the provides relations of package \a pkg which are in search state
  void incPvds(unsigned int pkg, KCudfWriter& wrt);
  /// Write all the _providers_ relations of package \a pkg which are in search state
  void incPvdrs(unsigned int pkg, KCudfWriter& wrt);
private:
  /// Set of packages that need to be initializated in search state
  std::set<int> init_search;
public:
  /// Default constructor
  KCudfReducer(void);
  /// Constructor taking the information for paranoid optimization
  KCudfReducer(std::istream& paranoid);
  /// Destructor
  virtual ~KCudfReducer(void);
  void package(unsigned int p, bool keep, bool install, const char*);
  /**
   * \brief Write the reduced problem using writers \a easy and \a
   * search. The return value is either \a RDO_SEARCH in case that a
   * search step is needed to solve the problem or \a RDO_SOL in the
   * case that the solution is found by the reducer.  statistics will
   * be available through \a st.
   */
  RD_OUT reduce(KCudfWriter& easy, KCudfWriter& search);
  const ReducerStats& stats() const;
  /// Return the state of a given package
  PKR_STATE state(unsigned int id) const;
  /// Return the number of safe providers for package \a p
  unsigned int getSP(unsigned int p) const;
  /// Return the number of candidate providers for package \a p
  unsigned int getCP(unsigned int p) const;
};

/**
 * \brief Convenience function to call the reducer with input file \a kcudf and to
 * output the information to \a solved and \a problem. The return value indicates the
 * if the problem does not have solution, is already solved or has to be solved.
 *
 * \todo throw an exception for the files!
 *
 * \todo take streams instead of file names, that ill make the code
 * more robust and lower the checks that need to be done.
 */
KCudfReducer::RD_OUT reduce(const char* kcudf, const char* solved,
                            const char* problem, const char* paranoid = NULL);
#endif
