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
#include <sstream>
#include <kcudf/reduce.hh>

namespace std {
  template <typename IteratorPair>
  auto begin(IteratorPair p) -> decltype(p.first) {
    return p.first;
  }
  template <typename IteratorPair>
  auto end(IteratorPair p) -> decltype(p.second) {
    return p.second;
  }
}

using namespace std;

/*
 * ReducerStats
 */

ReducerStats::ReducerStats(void)
  : pkgs(0), pkg_srch(0), pkg_is(0), pkg_slvd(0), pkg_nis(0), deps(0), confs(0),
  pvds(0), solution(false), fail(false) {}

std::ostream& operator <<(std::ostream& os, const ReducerStats& st) {
  if (st.fail) {
    os << "FAILURE: " << st.failure << endl;
    return os;
  }
  os <<  "General stats:" << endl
      << "\tSolution:\t" << (st.solution ? "yes" : "no") << endl
      << "Package stats:" << endl
      << "\tInitial packages:\t" << st.pkgs << endl
      << "\tPackages in search:\t" << st.pkg_srch << endl
      << "\tPackges solved:\t" << st.pkg_slvd << endl
      << "\tNot interesting packages:\t" << st.pkg_nis << endl
      << "\tInteresting packages:\t" << st.pkg_is << endl
      << "Package relations:" << endl
      << "\tDependencies:\t" << st.deps << endl
      << "\tConflicts:\t" << st.confs << endl
      << "\tProvides:\t" << st.pvds << endl;
  return os;
}

#ifndef NDEBUG
std::ostream& operator<< (std::ostream& o,const PKR_STATE st) {
  switch (st) {
  case PKR_CU: o << "CU"; break;
  case PKR_CI: o << "CI"; break;
  case PKR_MU: o << "MU"; break;
  case PKR_MI: o << "MI"; break;
  case PKR_SR: o << "SR"; break;
  case PKR_FL: o << "FL"; break;
  case PKR_AB: o << "AB"; break;
  }
  return o;
}

std::ostream& operator<< (std::ostream& o,const PK_OP op) {
  switch (op) {
  case PK_MU: o << "O_MU"; break;
  case PK_MI: o << "O_MI"; break;
  case PK_CI: o << "O_CI"; break;
  case PK_CU: o << "O_CU"; break;
  case PK_UCP: o << "O_UCP"; break;
  case PK_USP: o << "O_USP"; break;
  case PK_UPD: o << "O_UPD"; break;
  }
  return o;
}
#endif

/*
 * KCudfReducer
 */

PKR_STATE KCudfReducer::tf[5][4] =
  {
    // PK_MU   PK_MI   PK_CI   PK_CU
    {PKR_MU, PKR_MI, PKR_SR, PKR_CU}, // PKR_CU
    {PKR_MU, PKR_MI, PKR_CI, PKR_SR}, // PKR_CI
    {PKR_MU, PKR_FL, PKR_MU, PKR_MU}, // PKR_MU
    {PKR_FL, PKR_MI, PKR_MI, PKR_MI}, // PKR_MI
    {PKR_AB, PKR_AB, PKR_SR, PKR_SR}  // PKR_SR
  };

KCudfReducer::KCudfReducer(void) : GraphWriter() {}

KCudfReducer::KCudfReducer(std::istream& paranoid)
: GraphWriter() {
  int package;
  std::string line;
  while (paranoid.good()) {
    std::getline(paranoid,line);
    stringstream ss(line);
    ss >> package;
    init_search.insert(package);
  }
}

KCudfReducer::~KCudfReducer(void) {}

void KCudfReducer::package(unsigned int p, bool keep, bool install, const char* d) {
  // The initial state o each package depends on its state in the system.
  PKR_STATE st;
  if (keep) {
    if (install)
      st = PKR_MI;
    else
      st = PKR_MU;
  } else {
    if (install)
      st = PKR_CI;
    else
      st = PKR_CU;
  }
  pkg_st.insert(pkg_st.end(),make_pair(p,st));
  GraphWriter::package(p,keep,install,d);
  
  if (init_search.count(p) > 0) {
    //std::cout << "Package " << p << " forced to CI " << std::endl;
    addTask(PK_CI, p, TD_2); 
  }
 }

KCudfReducer::task_t
KCudfReducer::nextTask(void) {
  if (!todo1.empty()) {
    task_t t = todo1.front();
    todo1.pop_front();
    return t;
  }
  task_t t = todo2.front();
  todo2.pop_front();
  return t;
}

inline void
KCudfReducer::addTask(PK_OP op, unsigned int pk, KCudfReducer::TD_LST t) {
  task_t tk(op,pk);
  switch (t) {
  case TD_1:
    todo1.push_back(tk);
    break;
  case TD_2:
    todo2.push_back(tk);
    break;
  }
}

inline bool
KCudfReducer::workTodo(void) const {
  return !todo1.empty() || !todo2.empty();
}

#ifndef NDEBUG
void KCudfReducer::printWork(void) const {
  std::cerr << "Work in TODO1" << std::endl;
  for (task_t t : todo1) {
    std::cerr << "\tOp: " << t.first << " Pk: " << t.second << std::endl;
  }
  std::cerr << "Work in TODO2" << std::endl;
  for (task_t t : todo2) {
    std::cerr << "\tOp: " << t.first << " Pk: " << t.second << std::endl;
  }
}
#endif

bool KCudfReducer::isSP(PKR_STATE st) {
  assert(st < PKR_FL);
  return st == PKR_CI || st == PKR_MI;
}

bool KCudfReducer::isSPI(PKR_STATE st) {
  assert(st < PKR_FL);
  return isSP(st) || st == PKR_SR;
}

bool KCudfReducer::isCP(PKR_STATE st) {
  assert(st < PKR_FL);
  return st != PKR_MU;
}

inline void
KCudfReducer::update(unsigned int pid) {
  PKR_STATE st = state(pid);
  switch (st) {
  case PKR_MI:
    /*
      When a package must be installed, so they are its
      dependencies. Its conflicts must be make uninstallable.
    */
    for (unsigned int p : dependencies(pid)) {
      addTask(PK_MI, p, TD_1);
    }
    for (unsigned int p : conflicts(pid)) {
      addTask(PK_MU, p, TD_1);
    }
    break;
  case PKR_MU:
    /*
      When a package must be uninstalled, all the packages that
      depends on it must be uninstalled.
    */
    for (unsigned int p : dependers(pid)) {
      addTask(PK_MU, p, TD_1);
    }
    break;
  case PKR_CI:
    /*
      For a package to be installable, we need all its dependencies
      to be installable and all its conflicts to be uninstallable.
    */
    for (unsigned int p : dependencies(pid)) {
      addTask(PK_CI, p, TD_2);
    }
    for (unsigned int p : conflicts(pid)) {
      addTask(PK_CU, p, TD_2);
    }
    break;
  case PKR_CU:
    /*
      For a package to be uninstallable all the packages depending
      on it should become uninstallable.
    */
    for (unsigned int p : dependers(pid)) {
      addTask(PK_CU, p, TD_2);
    }
    break;
  case PKR_SR:
    /*
      When a package is in a search state it means that it could be
      either installed or not by the solver. In any case we need to
      guarantee that: all its dependencies can be installed
      (oterwise it wont make any sense to search for it), all its
      conflicts can be uninstalled and that all the packages
      depending on it can be uninstalled at some point (this is
      because the final result of the solver could be to uninstall
      the package).
    */
    for (unsigned int p : dependencies(pid)) {
      addTask(PK_CI, p, TD_2);
    }
    for (unsigned int p : conflicts(pid)) {
      addTask(PK_CU, p, TD_2);
    }
    for (unsigned int p : dependers(pid)) {
      addTask(PK_CU, p, TD_2);
    }
    break;
  default:
    assert(false);
    break;
  }
}

KCudfReducer::RD_OUT KCudfReducer::process(void) {
  // initialization
  for (unsigned int pid : packages()) {
    unsigned int c = 0;
    unsigned int s =0;
    for (unsigned int pvdr : providers(pid)) {
      PKR_STATE st_pvdr = state(pvdr);
      if (isSP(st_pvdr)) s++;
      if (isCP(st_pvdr)) c++;
    }
    assert(sp.count(pid) == 0);
    sp[pid] = s;
    cp[pid] = c;
    addTask(PK_UPD, pid, TD_1);
  }
  // reduction
  while (workTodo()) {
    PK_OP op;              // operation
    unsigned int pkgId;    // id of the package
    tie(op,pkgId) = nextTask();
    const PKR_STATE currState = state(pkgId);  // current state of the package

    if (op == PK_MU || op == PK_MI || op == PK_CI || op == PK_CU) {
      PKR_STATE nextState = tf[currState][op];
      if (nextState == PKR_FL) {
        std::stringstream ss;
        ss << pkgId << ": TF(" << currState << "," << op <<"): "
            << nextState << std::endl;
        st.fail = true;
        st.failure = ss.str();
        return RDO_FAIL;
      }
      assert(nextState != PKR_AB);
      if (currState != nextState) {
        if (!isSP(currState) && isSP(nextState))
          for (unsigned int p : provides(pkgId)) {
            assert(sp.count(p) > 0);
            sp[p] += 1;
          }
        if (isSP(currState) && !isSP(nextState))
          for (unsigned int p : provides(pkgId)) {
            assert(sp.count(p) > 0);
            sp[p] -= 1;
            if (sp.at(p) == 0 && isSPI(state(p))) {
              addTask(PK_USP, p, TD_2);
            }
          }
        if (!isSPI(currState) && isSPI(nextState) && sp.at(pkgId) == 0)
          addTask(PK_UPD, pkgId, TD_2);
        if (isCP(currState) && !isCP(nextState))
          for (unsigned int p : provides(pkgId)) {
            assert(cp.count(p) > 0);
            cp[p] -= 1;
            if (cp.at(p) <= 1)
              addTask(PK_UCP, p, TD_1);
          }
        state(pkgId,nextState);
        update(pkgId);
      }
    } else if (op == PK_UCP) {
      if (cp.at(pkgId) == 0)
        addTask(PK_MU, pkgId, TD_1);
      if (cp.at(pkgId) == 1) {
        for (unsigned int p : providers(pkgId)) {
          if (isCP(state(p)) && !dependency(pkgId,p)) {
            dependency(pkgId,p,NULL);
            addTask(PK_UPD, p, TD_1);
            addTask(PK_UPD, pkgId, TD_1);
          }
        }
      }
    } else if (op == PK_USP && sp.at(pkgId) == 0 && isSPI(state(pkgId))) {
      for (unsigned int p : providers(pkgId)) {
        addTask(PK_CI, p, TD_2);
      }
      addTask(PK_CU, pkgId, TD_2);
    } else if (op == PK_UPD) {
      update(pkgId);
      addTask(PK_UCP, pkgId, TD_1);
      addTask(PK_USP, pkgId, TD_2);
    }
  }

  st.pkgs = numPackages();
  return RDO_SEARCH;
}

void KCudfReducer::printPackages(std::ostream& os) const {
  unsigned int pci = 0, pcu = 0, pmi = 0, pmu = 0, psr = 0;

  for (unsigned int p : packages()) {
    switch (state(p)) {
    case PKR_SR: psr++; break;
    case PKR_MI: pmi++; break;
    case PKR_MU: pmu++; break;
    case PKR_CI: pci++; break;
    case PKR_CU: pcu++; break;
    case PKR_FL:
    case PKR_AB: assert(false); break;
    }
  }
  std::cerr << "Reducer statistics:" << std::endl
      << "\tCan Uninstall: " << pcu << std::endl
      << "\tCan Install: " << pci << std::endl
      << "\tMust Install: " << pmi << std::endl
      << "\tMust uninstall: " << pmu << std::endl
      << "\tSearch: " << psr << std::endl
      << "\tTotal packages: " << numPackages() << std::endl;
}

PKR_STATE KCudfReducer::state(unsigned int id) const {
  return pkg_st.at(id);
}

void KCudfReducer::state(unsigned int p, PKR_STATE st) {
  pkg_st[p] = st;
}

unsigned int
KCudfReducer::getSP(unsigned int p) const {
  assert(sp.count(p) > 0);
  return sp.at(p);
}

unsigned int
KCudfReducer::getCP(unsigned int p) const {
  assert(cp.count(p) > 0);
  return cp.at(p);
}

void KCudfReducer::incDeps(unsigned int pkg, KCudfWriter& wrt) {
  PKR_STATE p_st;
  for (unsigned int p : dependencies(pkg)) {
    p_st = state(p);
    if (p_st == PKR_SR) {
      //if (id != p) rep_deps++;  // avoid counting self dependencies
      wrt.dependency(pkg, p, "DEP-betweenSR");
      st.deps++;
    } else {
      /*
        All the dependencies that end up in a CI or MI state will be
        reported in the solved part as (Keep,Install) so we don't have
        to take them into account during the search.
      */
      assert(p_st == PKR_CI || p_st == PKR_MI);
    }
  }
}

void KCudfReducer::incConfs(unsigned int pkg, KCudfWriter& wrt) {
  PKR_STATE p_st;
  for (unsigned int p: conflicts(pkg)) {
    p_st = state(p);
    if (p_st == PKR_SR) {
      wrt.conflict(pkg, p, "CONF-betweenSR");
      st.confs++;
    } else {
      assert(p_st == PKR_CU || p_st == PKR_MU);
    }
  }
}

void KCudfReducer::incPvds(unsigned int pkg, KCudfWriter& wrt) {
  PKR_STATE p_st;
  for (unsigned int p: provides(pkg)) {
    p_st = state(p);
    if (p_st == PKR_SR) {
      wrt.provides(pkg, p, "PVD-betweenSR");
      st.pvds++;
    }
  }
}

void KCudfReducer::incPvdrs(unsigned int pkg, KCudfWriter& wrt) {
  PKR_STATE p_st;
  for (unsigned int p: providers(pkg)) {
    p_st = state(p);
    assert(p_st == PKR_SR || p_st == PKR_MU);
    if (p_st == PKR_SR) {
      wrt.provides(p,pkg,  "PVDR-SPI_SR");
      st.pvds++;
    }
  }
}

KCudfReducer::RD_OUT
KCudfReducer::reduce(KCudfWriter& solved, KCudfWriter& search) {
  cout << "*** Reducing ***" << endl;

  RD_OUT pr = process();

  if (pr == RDO_FAIL) {
    st.failure = true;
    return RDO_FAIL;
  }

  set<unsigned int> slvd;
  set<unsigned int> sp0;
  set<unsigned int> srch;

  // output information about packages
  for (unsigned int pkg: packages()) {
    PKR_STATE pkg_st = state(pkg);
    switch (pkg_st) {
    case PKR_AB:
    case PKR_FL:
      assert(false); break;
    case PKR_CI:
    case PKR_MI:
      if (sp.at(pkg) == 0) {
        /*
          in this case, the state of every package is MI or CI and
          we don't have yet a safe provider for it. Every package
          providing it should become part of the search.
        */
        search.package(pkg, true, true, "sp=0");
        srch.insert(srch.end(),pkg);
        sp0.insert(sp0.end(),pkg);
        st.pkg_is++;
        st.pkg_srch++;
      }
      /*
        Packages with any sp are considered solved but go in both
        outputs. Note that this case relies on the fact that the solver will
        ind a provider for those packages with sp = 0. This is why they are
        included in the solved part.
      */
      solved.package(pkg, true, true, "MI - CI");
      slvd.insert(slvd.end(),pkg);
      st.pkg_slvd++;
      break;
    case PKR_SR:
      /*
        Every package that ends up with a SR state will be in the
        search part of the output with the initial status it had at the
        beggining.
      */
      search.package(pkg, keep(pkg), install(pkg), "SR");
      srch.insert(srch.end(),pkg);
      st.pkg_srch++;
      break;
    case PKR_MU:
    case PKR_CU:
      /*
        we are not interested in these packages and this is why they are
        considered solved.
      */
      solved.package(pkg, true, false, "MU - CU");
      slvd.insert(slvd.end(),pkg);
      st.pkg_nis++;
      st.pkg_slvd++;
      break;
    }
  }

  // output information about relations
  for (unsigned int pkg : packages()) {
    PKR_STATE pkg_st = state(pkg);
    switch (pkg_st) {
    case PKR_AB:
    case PKR_FL: assert(false); break;
    case PKR_MU:
    case PKR_CU:
      // This package was reported as keep uninstall so we are not interested in any
      // relation involving it.
      break;
    case PKR_MI:
    case PKR_CI:
      /*
        For a package in MI or CI look all the packages it provides and if
        something is in search then it is considered self provided.
      */
//      foreach (unsigned int p, provides(pkg)) {
//        if (state(p) == PKR_SR) {
//          search.provides(p, p, "self-provided");
//        }
//      }
      /*
       When thereis no safe provider for a package, we need all their relations to be
       taken into account by the solver.
       */
      if(sp.at(pkg) == 0) {
        //incDeps(pkg,search);
        //incConfs(pkg,search);
        incPvdrs(pkg,search);
      }
    case PKR_SR:
      // Dependencies
      incDeps(pkg,search);
      // Conflicts
      incConfs(pkg,search);
      // Provides
      incPvds(pkg,search);
      break;
    }
  }

  cout << "*** Reducing [done]***" << endl;

  if ( srch.size() > 0)
    return RDO_SEARCH;
  st.solution = true;
  return RDO_SOL;
}

const ReducerStats& KCudfReducer::stats(void) const {
  return st;
}

