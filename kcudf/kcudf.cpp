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

#include <assert.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <kcudf/kcudf.hh>

unsigned int Package::next_id = 0;

Package::Package(bool inst, int v)
  : install(inst), keep(false), id(next_id), version(v), info(), keep_info() {
  next_id++;
}

Package::~Package(void) {}

unsigned int Package::getId(void) const {
  return id;
}

int Package::getVersion(void) const {
  return version;
}

void Package::addConflict(unsigned int p) {
  conflicts.insert(p);
}

void Package::addDependency(unsigned int p) {
  dependencies.insert(p);
}

const std::set<unsigned int>& Package::getDependencies(void) const {
  return dependencies;
}

const std::set<unsigned int>& Package::getConflicts(void) const {
  return conflicts;
}

void Package::markInstall(bool st) {
  if (install != st) {
    if (keep && !install) {
      std::cerr << "**warning: changing install for a already keep package"
                << getId() << " real version " << getVersion() << std::endl;
      //std::cerr << "Package keep info: " << getKeepInfo() << std::endl;
      assert(false);
    }
    install = st;
  }
}

void Package::markKeep(bool st) {
  keep = st;
}

bool Package::markedInstall(void) const {
  return install;
}

bool Package::markedKeep(void) const {
  return keep;
}

const char* Package::getInfo(void) const {
  return info.c_str();
}

const char* Package::getKeepInfo(void) const {
  return keep_info.c_str();
}

void Package::addInfo(const char* ninf) {
  info.append(" -=- ");
  info.append(ninf);
}

void Package::addKeepInfo(const char* ninf) {
  keep_info.append(" -=- ");
  keep_info.append(ninf);
}

// SelfPackage
SelfPackage::SelfPackage(const std::string& name, bool inst, int v)
  : Package(inst,v), nm(name) {
  assert(v >= 0);
  std::stringstream ss;
  ss << name << "v" << v;
  info = ss.str();
}

const std::string& SelfPackage::name(void) const {
  return nm;
}

SelfPackage::~SelfPackage(void) {}

bool SelfPackage::isConcrete(void) const {
  return true;
}

// Disjunction
Disjunction::Disjunction(const char* inf)
  : Package(false,-1), forwarded(false), fwd(NULL), conf_but(0), has_but(false), flt(false) {
  /*
    Disjunctions are not versioned, this is why we pass -1 as version to the Package
    constructor.
  */
  info.append("disj-").append(inf);
}

Disjunction::Disjunction(int v, const char* inf)
  : Package(false,v), forwarded(false), fwd(NULL), conf_but(0), has_but(false), flt(false) {
  info.append("disj-").append(inf);
}

Disjunction::~Disjunction(void) {}

void Disjunction::addConflict(unsigned int p) {
  if (forwarded)
    fwd->addConflict(p);
  else
    Package::addConflict(p);
}

void Disjunction::addDependency(unsigned int p) {
  if (forwarded)
    fwd->addDependency(p);
  else
    Package::addDependency(p);
}

void Disjunction::addProvider(unsigned int p) {
  if (forwarded)
    assert(false);
  else
    providers.insert(p);
}

std::set<unsigned int>& Disjunction::getProviders(void) {
  if (forwarded) {
    assert(!fwd->isConcrete());
    return static_cast<Disjunction*>(fwd)->getProviders();
  }
  return  providers;
}

const std::set<unsigned int>& Disjunction::getProviders(void) const {
  if (forwarded) {
    assert(!fwd->isConcrete());
    return static_cast<Disjunction*>(fwd)->getProviders();
  }
  return  providers;
}

void Disjunction::addBut(unsigned int p) {
  //assert(!hasBut());
  conf_but = p;
  has_but = true;
}

bool Disjunction::hasBut(void) const {
  return has_but;
}

unsigned int Disjunction::but(void) const {
  assert(hasBut()); // todo: make it an exception
  return conf_but;
}

bool Disjunction::isConcrete(void) const {
  if (forwarded)
    return fwd->isConcrete();
  return false;
}

unsigned int Disjunction::getId(void) const {
  if (forwarded)
    return fwd->getId();
  return Package::getId();
}

int Disjunction::getVersion(void) const {
  if (forwarded)
    return fwd->getVersion();
  return Package::getId();
}

void Disjunction::markInstall(bool st) {
  if (forwarded)
    fwd->markInstall(st);
  else
    Package::markInstall(st);
}

bool Disjunction::markedInstall(void) const {
  if (forwarded) return fwd->markedInstall();
  return Package::markedInstall();
}

void Disjunction::markKeep(bool st) {
  if (forwarded)
    fwd->markKeep(st);
  else
    Package::markKeep(st);
}

bool Disjunction::markedKeep(void) const {
  if (forwarded) return fwd->markedKeep();
  return Package::markedKeep();
}

bool Disjunction::isFlat(void) const {
  if (forwarded) {
    assert(!fwd->isConcrete());
    return  static_cast<Disjunction*>(fwd)->isFlat();
  }
  return flt;
}

void Disjunction::flat(std::map<unsigned int,Package*>& pkgs) {
  if (forwarded) {
    assert(!fwd->isConcrete());
    return  static_cast<Disjunction*>(fwd)->flat(pkgs);
  }
  if (flt) return;

  // first flat all the providers of this disjunction
  for (unsigned int p : getProviders()) {
    if (!pkgs[p]->isConcrete()) {
      static_cast<Disjunction*>(pkgs[p])->flat(pkgs);
    }
  }

  // all the providers has been flatten, flat the package itself
  std::set<unsigned int> toAdd;
  for (unsigned int p : getProviders()) {
    if (!pkgs[p]->isConcrete()) {
      Disjunction *d = static_cast<Disjunction*>(pkgs[p]);
      assert(d->flt);
      for (unsigned int rp : d->getProviders()) {
        assert(pkgs[rp]->isConcrete());
        toAdd.insert(rp);
      }
    } else {
      toAdd.insert(p);
    }
  }
  getProviders().clear();
  for (unsigned int p : toAdd){
    assert(pkgs[p]->isConcrete());
    getProviders().insert(p);
  }

  if (has_but)
    getProviders().erase(conf_but);

  // mark as flatten
  flt = true;
}

void Disjunction::setForward(Package *p) {
  if (forwarded) {
    assert(!fwd->isConcrete());
    return  static_cast<Disjunction*>(fwd)->setForward(p);
  }
  assert(p->getId() != getId());

  // the dependencies of this are added as dependencies of p
  for (unsigned int d : getDependencies()) {
    p->addDependency(d);
  }

  // the conflicts of this are added as conflicts of p
  for (unsigned int c : getConflicts()) {
    p->addConflict(c);
  }

  if (getProviders().count(p->getId()) > 0) {
    getProviders().erase(p->getId());
  }


  // forward the information in this package
  std::stringstream ss,si;
  ss << "[(" << getId() << ") " << getInfo() << "]";
  if (!keep_info.empty()) {
    si << "[(" << getId() << ") " << getKeepInfo() << "]";
  }

  fwd = p;
  forwarded = true;
  fwd->addInfo(ss.str().c_str());
  fwd->addKeepInfo(si.str().c_str());
  std::stringstream sf;
  sf << "  -fwd-> " << p->getId();
  info.append(sf.str());
}

// Translator statistics
TranslatorStats::TranslatorStats(void)
  : cp(0), rd(0), ed(0), zp(0), fail(false) {}

// KCudfData
KCudfData::KCudfData(const CudfDoc& doc, TranslatorStats& stats) {
  /*
    first pass, get all the information about concrete packages, the
    current status of the packages is stored at this point (whether
    it is installed or not)
  */
  processConcretePackages(doc);
  processInstalledPackages(doc);
  /*
    second pass: Equality constraints are parsed and added to sepcv
    data structure. This will store all the information needed to
    solve further constraints.
  */
  processEqualityConstraints(doc);
  processProvides(doc);
  /// In this pass of the document we are only interested in range constraints
  processRangeConstraints(doc);

  // Flat all the disjunction packages
  for (auto p = packages.begin(); p != packages.end(); ++p) {
    if (!p->second->isConcrete()) {
      Disjunction *d = static_cast<Disjunction*>(p->second);
      d->flat(packages);
    }
  }

  // Try to compress disjunctions
  /*
    Concrete packages are put first to try to get rid of the disjunction associated with
    every concrete. After that real disjunctions are tackled. A concrete package on
    the tree is encoded as a disjunction that has itself as the only provider.
  */
  unsigned int compressed = 0;
  DTNode *dt = new DTNode();
  std::set<unsigned int> pvd;
  for (auto p = packages.begin(); p != packages.end(); ++p) {
    if (p->second->isConcrete()) {
      unsigned int id = p->second->getId();
      pvd.insert(id);
      // TODO: probably we can offer a way to add a disjunction with only one
      // provider and avoid creating a set.
      unsigned int nid = dt->addDisjunction(p->second->getId(),pvd);
      (void)nid; // avoid a compiler warning when RELEASE mode
      assert(nid == id);
      pvd.erase(id);
      assert(pvd.empty());
    }
  }

  for (auto p = packages.begin(); p != packages.end(); ++p) {
    if (!p->second->isConcrete()) {
      Disjunction *d = static_cast<Disjunction*>(p->second);
      unsigned int nid = dt->addDisjunction(d->getId(),d->getProviders());
      if (nid != d->getId()) {
        d->setForward(packages[nid]);
        compressed++;
      }
    }
  }

  /*
    Up to this point the install field of every package reflects its
    state in the current installation. From now on this field and the
    keep one will be used to encode the problem for the solver.

    Before this point no function should alter the installed field of
    the package to encode something.
  */

  unsigned int zero_prov = 0;
  for (auto p = packages.begin(); p != packages.end(); ++p) {
    if (!p->second->isConcrete()) {
      Disjunction *d = static_cast<Disjunction*>(p->second);
      // disjunction with only one provider are also forwarded to the provider itself
      if (d->getProviders().size() == 1) {
        assert(false);
      } else if (d->getProviders().size() == 0) {
        d->markInstall(false);
        d->markKeep(true);
        d->addKeepInfo("keep x zero providers");
        zero_prov++;
      }
    }
  }

  // Fixing virtuals is something that can be only done _after_ falttening all disjunctions.
  fixInstallVirtuals();
  /// Process the upgrade part of the request
  processRequest(doc,dt);
  fixInstallVirtuals();
  delete dt;

	std::cout << "Is initial installation consistent? " << (consistent() ? "yes" : "no") << std::endl;
  
  unsigned int disj = 0;
  for (auto p = packages.begin(); p != packages.end(); ++p) {
    if (!p->second->isConcrete()) {
      disj++;
    }  
	}

  // statistics info
  stats.cp = concrete.size();
  stats.rd = disj;
  stats.ed = compressed;
  stats.zp = zero_prov;
  
  // fill in bigPackages
  for (auto p = packages.begin(); p != packages.end(); ++p) {
    Package *pi = p->second;
    if (pi->isConcrete() && pi->markedInstall()) {
      Package *rp = packages.at(pi->getId());
      SelfPackage *pk = static_cast<SelfPackage*>(rp);
      crtPackages_.push_back(pk->getId());
      std::stringstream ss;
      ss << pk->name() << "-pvany";
      assert(constv.count(ss.str()) > 0);
      Package *d = packages[constv[ss.str()]];
      bigPackages_.insert(bigPackages_.end(),d->getId());
      //std::cout << "Marked install " << pk->name() << " " << d->getId() << std::endl;
    }
  }
   
}

void KCudfData::processConcretePackages(const CudfDoc& doc) {
  for (const CudfPackage& pi : doc.getPackages()) {
    //std::cerr << "reaing info about package " << pi->name() << std::endl;

    /**
       As every element in the document is a concrete package then
       this are the kind of packages created at this point. When a
       concrete package is created it is registeres in the \a concrete data
       structure. A disjunction on specV is also created and the only provider
       at this time for it is the concrete package.
    */
    // create the concrete package and register it
    SelfPackage *p = new SelfPackage(pi.name(),pi.installed(),pi.version());
    // the package should not exist
    if (concrete.count(pi.name()) > 0) {
      assert(concrete[pi.name()].count(pi.version()) == 0);
    }
    concrete[pi.name()][pi.version()] = p->getId();
    // register it on packages
    packages[p->getId()] = p;

    // create a disjunction and register it
    std::stringstream ss;
    ss << "(=" << pi.version() << ")" << pi.name();
    Disjunction *d = new Disjunction(ss.str().c_str());

    p->addDependency(d->getId());
    d->addProvider(p->getId());

    packages[d->getId()] = d;

    std::stringstream sall;
    sall << pi.name() << "-pvall";
    Disjunction *all = getDisjunction(sall.str());

    d->addProvider(all->getId());
    all->addDependency(d->getId());

    packages[all->getId()] = all;

    // add the disjunction to specv
    specv[pi.name()][pi.version()] = d->getId();
    
    // the following code brings support for the paranoid optimization criteria.
    if (pi.installed()) {
      // if the package is installed we create a pkgname-pvany disjunction with the
      // package as a provider. This disjunction will represent the global package
      // and is installed if at least one of the corresponding package units are
      // installed. At this point we only create the disjunction and latter on in
      // processInstalledPackages this disjunction will be filled with providers
      std::stringstream sany;
      sany << pi.name() << "-pvany";    
      Disjunction *any = getDisjunction(sany.str());
      any->addProvider(all->getId());
      all->addDependency(any->getId());
    }
  }
}

void KCudfData::processInstalledPackages(const CudfDoc& doc) {
  // when this method is called the following assumptions are met:
  // 1) All the concrete packages has been processed (e.g. there is an entry in
  //    packages for all of them)
  // 2) There is a pkgname-any disjunction already created for those packages
  //    that were found installed.
  //
  // As a result, this method will add providers for all the packages for whose
  // corresponding pkgname-any disjunction exists.
  for  (const CudfPackage& pi : doc.getPackages()) {
    unsigned int pi_id = specv[pi.name()][pi.version()];
    
    std::stringstream sany;
    sany << pi.name() << "-pvany";    
    auto any = constv.find(sany.str());
    if (any != constv.end()) {
      // There is an any disjunction so this package is its provider.
      Disjunction * dsj = static_cast<Disjunction*>(packages[any->second]);
      dsj->addProvider(pi_id);
      Package *p = packages[pi_id];
      p->addDependency(dsj->getId());
    }
  }
}

void KCudfData::processEqualityConstraints(const CudfDoc& doc) {
  // for the packages stated in the universe
  for (const CudfPackage& pi : doc.getPackages()) {
    // the id of the current package
    unsigned int cpi_id = concrete[pi.name()][pi.version()];
    unsigned int pi_id = specv[pi.name()][pi.version()];

    // process the keep version feature of the packages that have it.
    if (pi.keep() == KP_VERSION) {
      std::cerr << "Keep version in package " << pi << std::endl;
      assert(pi.installed()); // todo: convert to an exception
      Package *p = packages[pi_id];
      p->markInstall(true);
      p->markKeep(true);
      p->addKeepInfo("keep version");
    }

    // process the conflicts of the current package
    for (const Vpkg& vpki : pi.conflicts()) 
      if (vpki.getRel() == ROP_EQ) {
        Package *p = addDisjunction(vpki.getName(), vpki.getVersion());
        packages[cpi_id]->addConflict(p->getId());
      }

    // process the dependencies
    for  (const vpkglist_t& cni : pi.depends())
      // i-th conjunction term
      for (const Vpkg& djj : cni)
      //j-th disjunction term
      if (djj.getRel() == ROP_EQ) {
        Package *p = addDepDisjunction(djj.getName(), djj.getVersion());
        // the dependency relation starts at the concrete package and ends at
        // the disjunction of the other concrete.
        packages[cpi_id]->addDependency(p->getId());
      }
    
    // process the provides
    for (const Vpkg& vpki : pi.provides())
      if (vpki.getRel() == ROP_EQ) {
        Package *p = addDisjunction(vpki.getName(), vpki.getVersion());
        // add the current package as a provider of that disjunction
        assert(!p->isConcrete());
        Disjunction *d = static_cast<Disjunction*>(p);

        d->addProvider(cpi_id);
        packages[cpi_id]->addDependency(cpi_id);
        
        // a provide all is a provided of the created disjunction
        std::stringstream sall;
        sall << vpki.getName() << "-pvall";
        Disjunction *al = getDisjunction(sall.str());
        d->addProvider(al->getId());
        al->addDependency(d->getId());
      }
  }

  // for the packages stated in the request

  /*
    There can be two possibilities for equality constraints in the
    request: they can refer to concrete packages or they can referr to
    virtual packages, in any case, the information about those
    packages was collected in during the processing of the concrete
    packages and then they must be stored in \a specv. If they are not
    then the package or the version mensioned in the request does not
    exist and: try to remove the package will result in no action but
    trying to install or upgrade it will result in a failure.
  */

  for (const Vpkg& vpk : doc.reqToInstall()) {
    if (vpk.getRel() == ROP_EQ) {
      std::cerr << "Requested to install (EQ) " << vpk << std::endl;
      addDisjunction(vpk.getName(),vpk.getVersion());
    }
  }

  for (const Vpkg& vpk : doc.reqToRemove()) {
    if (vpk.getRel() == ROP_EQ) {
      std::cerr << "Requested to remove (EQ) " << vpk << std::endl;
      addDisjunction(vpk.getName(), vpk.getVersion());
    }
  }
}

void KCudfData::processProvides(const CudfDoc& doc) {
  for (const CudfPackage& pi : doc.getPackages()) {
    // the id of the current package
    unsigned int cpi_id = concrete[pi.name()][pi.version()];
    // the package pointer corresponding to the current package
    Package *cpi_pkg = packages[cpi_id];

    for (const Vpkg& vpki : pi.provides())
      if (vpki.versioned()) {
        if (vpki.getRel() != ROP_EQ) {
          /*
            CUDF semantics only allows packages to specify a general
            (unconstrained) vpkg in the provide statement or one with
            an specific version and a equality.
          */
          std::ostringstream ss;
          ss << "Bad provided description: " << vpki
             << ": only unconstrained and equality constrained expressions are allowed here";
          ss << std::endl << "While parsing package: " << std::endl << pi  << std::endl;
          throw KCudfInvalidProvide(ss.str().c_str());
        }
        // in the other case, the provided statement was already
        // parsed in processEqualityConstraints
      } else {
        /*
          An unconstrained provide is intended to provide everything that matches
          the name. In this case we create two nodes, one representing the "all"
          (if it does not exist) and the other representing the "any". The all is
          a provider of the any.
        */
        std::ostringstream ss;
        ss << vpki.getName() <<  "-pvall";
        Disjunction *all = getDisjunction(ss.str());
        all->addProvider(cpi_id);
        cpi_pkg->addDependency(all->getId());
      }
  }
}

void KCudfData::processRangeConstraints(const CudfDoc& doc) {
  for (const CudfPackage& pi : doc.getPackages()) {
    /*
      for the current package we have several nodes representing it in the graph,
      the first one is a node representing the concrete package itself (cpi_id) and
      the second one is the node representing a disjunction for it (pi_id).
    */
    // the id of the current package
    unsigned int cpi_id = concrete[pi.name()][pi.version()];
    // the package pointer corresponding to the current package
    Package *cpi_pkg = packages[cpi_id];

    // process the conflicts of the current package
    for (const Vpkg& vpki: pi.conflicts())
      if (vpki.getRel() != ROP_EQ) {
        Disjunction *d_any = getDepDisjunction(vpki);

        // handling self conflict
        std::ostringstream ss;
        ss << vpki.getName() << "-any\\" << pi.name() << "="  << pi.version();
        const std::string& sb = ss.str();
        Disjunction *d = getDisjunction(sb);
        d->addProvider(d_any->getId());
        d->addBut(cpi_id);

        // Add the not-but as a conflict of the current parsed package
        cpi_pkg->addConflict(d->getId());
      }

    // process the dependencies
    for (const vpkglist_t& cni: pi.depends()) {
      // i-th conjunction term
      for (const Vpkg& djj : cni)
        //j-th disjunction term
        if (djj.getRel() != ROP_EQ) {
          getDepDisjunction(djj);
        }

      // if there is a real disjunction
      if (cni.size() > 1) {
        //std::cerr << "Found disjunction " << *cni << std::endl;
        std::ostringstream ss; ss  <<  cni;
        const std::string& s = ss.str();
        if (constv.count(s) > 0) {
          // The disjunction was already processed
          //std::cerr << "Disjunction Already there!! " << s << std::endl;
          cpi_pkg->addDependency(constv[s]);
        } else {
          //std::cerr << "new disjunction for " << s << std::endl;
          Disjunction *p = newDisjunction(s);
          for (const Vpkg& djj: cni) {
            //std::cerr << "Term in disjunction " << djj << std::endl;
            std::stringstream ss;
            if (!djj.versioned())
              ss << djj.getName() << "-pvany";
            else {
              ss << djj.serialize();
            }
            const std::string& ts = ss.str();//djj->serialize();
            assert(constv.count(ts) > 0 ||
                   (specv.count(djj.getName()) > 0 
                    && specv[djj.getName()].count(djj.getVersion()) > 0));
            /* At this point all the elements of a disjunction have to
               be parsed as virtuals or concrete packages */
            if(constv.count(ts) > 0) {
              // the term is a virtual
              p->addProvider(constv[ts]);
            } else if (specv.count(djj.getName()) > 0 && 
                       specv[djj.getName()].count(djj.getVersion()) > 0) {
              // the term is a concrete package
              p->addProvider(specv[djj.getName()][djj.getVersion()]);
            } else {
              std::cerr << "Unknown (unparsed) term in disjunction: " << ts << std::endl;
              assert(false);
            }
          }
          // At this point the disjunction is completelly
          cpi_pkg->addDependency(p->getId());
        }
      } else {
        // only one term in the disjunction
        std::ostringstream ss;
        auto vcni = *(cni.begin());
        if (vcni.versioned())
          ss  <<  vcni;
        else
          ss << vcni.getName() << "-pvany";
        const std::string& s = ss.str();
        //std::cerr << "There is only one term in the dependency " << s;
        auto entry = constv.find(s);
        if (entry != constv.end()) {
          // there is an entry for this in the disjunctions
          //std::cerr << " found as disjunction" << std::endl;
          cpi_pkg->addDependency(entry->second);
        } else {
          // todo: throw an exceptions
          const std::string& ps = cni.begin()->getName();
          unsigned int pv = cni.begin()->getVersion();
          assert(specv.count(ps) > 0); // the package must be present as real
          auto int_map = specv[ps];
          assert(int_map.count(pv) > 0); // the package must be present as real
          //std::cerr << " found as real" << std::endl;
          cpi_pkg->addDependency(int_map[pv]);
        }
      }
    }

    // for the packages stated in the request
    for (const Vpkg& vpk: doc.reqToInstall())
      if (vpk.getRel() != ROP_EQ) {
        //std::cerr << "Requested to install (RANGE) " << *vpk << std::endl;
        getDisjunction(vpk);
      }

    for (const Vpkg& vpk : doc.reqToRemove()) 
      if (vpk.getRel() != ROP_EQ) {
        //std::cerr << "Requested to remove (RANGE) " << *vpk << std::endl;
        getDisjunction(vpk);
      }
  }
}

void KCudfData::fixInstallVirtuals(void) {
  for (auto p = constv.begin(); p != constv.end(); ++p) {
    Disjunction *d = static_cast<Disjunction*>(packages[p->second]);
    if (!d->isConcrete())
      for (unsigned int i: d->getProviders()) {
        assert(packages[i]->isConcrete());
        if (packages[i]->markedInstall())
          d->markInstall(true);
      }
  }
}

void KCudfData::processRequest(const CudfDoc& doc, DTNode *dsj ) {
  std::set<Package*> toInstall;
  std::set<Package*> toUninstall;

  // process keep constraints for package and feature values.
  for (const CudfPackage& pi: doc.getPackages()) {
  //for (BOOST_AUTO(pi, doc.pkg_begin()); pi != doc.pkg_end(); ++pi) {
    // process the keep version feature of the packages that have it.
    switch (pi.keep()) {
    case KP_PACKAGE:
      {
        std::cerr << "Keep package constraint found" << std::endl;
        // keep at least one concrete package with the same name
        std::set<unsigned int> range;
        auto int_map = concrete[pi.name()];
        for (auto p = int_map.begin(); p != int_map.end(); ++p) {
          range.insert(p->second);
        }
        if (range.size() > 1) {
          // we have to create a disjunction
          std::ostringstream name; name << pi.name() << "-keep-pkg";
          if (constv.count(name.str()) > 0) {
            std::cerr << "This keep was already parsed: " << pi << std::endl;
          } else {
            Disjunction *d = getDisjunction(name.str());
            for (unsigned int i: range) {
              d->addProvider(i);
            }
            d->flat(packages);
            unsigned int nid = dsj->addDisjunction(d->getId(),d->getProviders());
            if (nid != d->getId())
              d->setForward(packages[nid]);
            toInstall.insert(d);
          }
        } else {
          // there is just one package with that name and is this one so this is
          // equivalent to a keep:version
          std::cerr << "equivalent to keep:version" << std::endl;
          toInstall.insert(packages[concrete[pi.name()][pi.version()]]);
        }
      }
      break;
    case KP_FEATURE:
      {
        std::cerr << "Keep feature constraint found" << std::endl;
        // this case is only valid if there is at least one provide statement in
        // the package definition.
        assert(pi.provides().size() > 0);
        for (const Vpkg& vpki: pi.provides()) {
          if (vpki.versioned() && vpki.getRel() == ROP_EQ) {
            std::cerr << "keep feature versioned" << std::endl;
            assert(specv.count(vpki.getName()) > 0);
            assert(specv[vpki.getName()].count(vpki.getVersion()) > 0);
            toInstall.insert(packages[specv[vpki.getName()][vpki.getVersion()]]);
          } else {
            std::cerr << "keep feature general" << std::endl;
            assert(!vpki.versioned());
            std::stringstream ss;
            ss << vpki.getName() << "-pvany";
            const std::string& s = ss.str();
            assert(constv.count(s) > 0);
            toInstall.insert(packages[constv[s]]);
          }
        }
      }
      break;
    case KP_VERSION:
      // this case was already handled during equality constraint processing
      std::cerr << "Keep version constraint found" << std::endl;
      assert(packages[concrete[pi.name()][pi.version()]]->markedKeep());
      assert(packages[concrete[pi.name()][pi.version()]]->markedInstall());
      break;
    case KP_NONE:
      //std::cerr << "Keep none constraint found" << std::endl;
      break;
    }
  }
  /*
    during the processing of the request no package is going to be
    marked, instead they are put in the corresponding set and at the
    very end those set are traversed and real packages are marked.
  */

  // Process the upgrade
  for (const Vpkg& vpk: doc.reqToUpgrade()) {
    std::cerr << "Requested to upgrade constraint " << vpk << std::endl;
    std::stringstream name; name << vpk.serialize() << "-req-upg";
    Disjunction *upg = new Disjunction(name.str().c_str());
    packages[upg->getId()] = upg;
    if (constv.count(name.str()) > 0) {
      std::cerr << "Request constraint already aprsed " << vpk << std::endl;
    } else {
      // 1- check for the provideall, if it exist and is installed then we fail.
      std::stringstream ss;
      ss << vpk.getName() << "-pvall";
      const std::string& s = ss.str();
      if (constv.count(s) > 0) {
        std::cerr << "there is a provide all" << std::endl;
        Package *p = packages[constv[s]];
        if (p->markedInstall()) {
          std::ostringstream se;
          se << "Unable to fulfill request for: " << vpk
             << ": asked to upgrade it but a package providing all the versions is installed.";
          throw KCudfFailedRequest(se.str().c_str());
        } else {
          // we have to avoid the provideall from being installed
          toUninstall.insert(p);
        }
      }
      // 2- take all the specific versions of the package name
      assert(specv.count(vpk.getName()) > 0);
      auto int_map = specv[vpk.getName()];
      bool interested = true;
      std::set<unsigned int> range;
      PkUnit pu(vpk.getName(),-1);
      for (auto pi = int_map.rbegin(); pi != int_map.rend(); ++pi) {
        /*
          pi is an element of specv so it corresponds to a disjunction.
        */
        Package *p = packages[pi->second];
        unsigned int curr_version = p->getVersion();
        assert(curr_version > 0);
        pu.version(curr_version);
        if ((pu && vpk) && interested) {
          //std::cerr << "This is a provider: " << curr_version << " for " << *vpk << std::endl;
          range.insert(p->getId());
          interested = !p->markedInstall();
        } else {
          //std::cout << "We are not interested in this package anymore " << curr_version << " of " << *vpk << std::endl;
          toUninstall.insert(p);
        }
      }
      // 3- Create a conflict among all the packages in range (at most one should be installed at the end)
      pairwiseConflicting(range);
      // 4- create a disjunction with the elements in range as providers (if it
      //    does not exist yet
      Disjunction *tmp = new Disjunction("temporal");
      for (unsigned int i: range) {
        tmp->addProvider(i);
      }
      tmp->flat(packages);
      for (unsigned int i: tmp->getProviders()) {
        std::cerr << "Possible provider: " << i << " ";
      }
      std::cerr << std::endl;
      // if this disjunction already exist we don't need to register a new one
      unsigned int d_id = dsj->addDisjunction(tmp->getId(),tmp->getProviders());
      if (d_id != tmp->getId()) {
        std::cerr << "upgrade: Already existent disjunction " << d_id << std::endl;
        upg->addProvider(d_id);
      } else {
        std::cerr << "upgrade: Newly existent disjunction" << std::endl;
        packages[tmp->getId()] = tmp;
        upg->addProvider(tmp->getId());
      }
      // 5. mark the upgrade package as being installed
      toInstall.insert(upg);
    }
  }

  /*
    Process the install:

    All the install statements should be already in constv (produced
    by range constraints). Then the only thing remaining is to mark those
    packages as keep install.
  */
  for (const Vpkg& vpk: doc.reqToInstall()) {
    if (vpk.getRel() == ROP_EQ) {
      assert(specv.count(vpk.getName()) > 0);
      Package *p = packages[specv[vpk.getName()][vpk.getVersion()]];
      toInstall.insert(p);
      p->addKeepInfo("requested to install");
    } else {
      std::stringstream ss;
      if (vpk.versioned())
        ss << vpk.serialize();
      else
        ss << vpk.getName() << "-pvany";
      const std::string& name = ss.str();
      assert(constv.count(name) > 0);
      Package *p = packages[constv[name]];
      toInstall.insert(p);
      p->addKeepInfo("Requested to install - cst");
    }
  }

  // Process the removals
  for (const Vpkg& vpk: doc.reqToRemove()) {
    if (vpk.getRel() == ROP_EQ) {
      assert(specv.count(vpk.getName()) > 0);
      toUninstall.insert(packages[specv[vpk.getName()][vpk.getVersion()]]);
    } else {
      const std::string& name = vpk.serialize();
      assert(constv.count(name) > 0);
      toUninstall.insert(packages[constv[name]]);
    }
  }

  // Mark packages
  for (Package *i: toInstall) {
    if (i->markedKeep() && !i->markedInstall()) {
      std::ostringstream se;
      se << "Unable to fulfill request for: " << i->getInfo()
         << " info: " << i->getKeepInfo();
      throw KCudfFailedRequest(se.str().c_str());
    }
    i->markInstall(true);
    i->markKeep(true);
  }
  for (Package *i: toUninstall) {
    i->markInstall(false);
    i->markKeep(true);
  }
}

Package* KCudfData::addDepDisjunction(const std::string& name, unsigned int version) {
  if (specv.count(name) > 0) {
    // do we have a matching version?
    auto int_map = specv[name];
    auto p = int_map.find(version);
    if (p != int_map.end()) {
      //std::cerr << "Concrete package found constraint " << name << " " << version << std::endl;
      return packages[p->second];
    }
  }
  std::stringstream ss;
  ss << name << "=" << version;
  Disjunction *p = new Disjunction(version,ss.str().c_str());
  packages[p->getId()] = p;
  specv[name][version] = p->getId();

  std::stringstream pvdallss;
  pvdallss << name << "-pvall";
  Disjunction *all = getDisjunction(pvdallss.str());
  //assert(constv.count(pvdallss.str()) > 0);
  //p->addProvider(constv[pvdallss.str()]);
  p->addProvider(all->getId());
  return p;
}

Package* KCudfData::addDisjunction(const std::string& name, unsigned int version) {
  //  let's see if we have a concrete package matching the name
  if (specv.count(name) > 0) {
    // do we have a matching version?
    auto int_map = specv[name];
    auto p = int_map.find(version);
    if (p != int_map.end()) {
      //std::cerr << "Concrete package found constraint " << name << " " << version << std::endl;
      return packages[p->second];
    }
  }
  // if we did not found a matching package then we have to add a new
  // disjunction because it is a provided thing (we expect it to be)
  std::stringstream ss;
  ss << name << "=" << version;
  Disjunction *p = new Disjunction(version,ss.str().c_str());
  packages[p->getId()] = p;
  specv[name][version] = p->getId();
  //std::cerr << "Virtual package added " << name << " = " << version << std::endl;
  return p;
}

Disjunction* KCudfData::newDisjunction(const std::string& s) {
  Disjunction *p = new Disjunction(s.c_str());
  packages[p->getId()] = p;
  constv[s] = p->getId();
  return p;
}

Disjunction* KCudfData::getDisjunction(const std::string& name) {
  auto d = constv.find(name);
  if (d == constv.end()) {
    // it does not exist, create one
    Disjunction *p = newDisjunction(name);
    return p;
  }
  assert(constv.count(name) > 0);
  return static_cast<Disjunction*>(packages[d->second]);
}

Disjunction* KCudfData::getDepDisjunction(const Vpkg& cs) {
  assert(cs.getRel() != ROP_EQ);
  std::stringstream ss;
  if (!cs.versioned()) {
    ss << cs.getName() << "-pvany";
  } else {
    ss << cs.serialize();
  }
  const std::string& name = ss.str();
  auto d = constv.find(name);
  if (d == constv.end()) {
    // it does not exist, create one
    Disjunction *p = newDisjunction(name);
    /// solve the constraint and add the providers to p
    std::list<unsigned int> l; solveConstraint(cs,l);
    for (auto pi = l.begin(); pi != l.end(); ++pi) {
      p->addProvider(*pi);
    }
    std::stringstream csall;
    csall << cs.getName() << "-pvall";
    if (constv.count(csall.str()) > 0) {
      p->addProvider(packages[constv[csall.str()]]->getId());
    }
    return p;
  }
  assert(constv.count(name) > 0);
  return static_cast<Disjunction*>(packages[d->second]);
}


Disjunction* KCudfData::getDisjunction(const Vpkg& cs) {
  assert(cs.getRel() != ROP_EQ);
  std::stringstream ss;
  if (!cs.versioned()) {
    ss << cs.getName() << "-pvany";
  } else {
    ss << cs.serialize();
  }
  const std::string& name = ss.str();
  auto d = constv.find(name);
  if (d == constv.end()) {
    // it does not exist, create one
    Disjunction *p = newDisjunction(name);
    /// solve the constraint and add the providers to p
    std::list<unsigned int> l; solveConstraint(cs,l);
    for (auto pi = l.begin(); pi != l.end(); ++pi) {
      p->addProvider(*pi);
    }
    return p;
  }
  assert(constv.count(name) > 0);
  return static_cast<Disjunction*>(packages[d->second]);
}

void KCudfData::solveConstraint(const Vpkg& c, std::list<unsigned int>& pkgs) const {
  // here we have to solve the constraint c based on the information specv.
  if ( specv.count(c.getName()) > 0 ) {
    // there is an entry with the name in specv
    auto int_map = specv.find(c.getName())->second;
    PkUnit pu(c.getName(),-1);
    for (auto pp = int_map.begin(); pp != int_map.end(); ++pp) {
      pu.version(pp->first);
      if (pu && c)
        pkgs.push_back(pp->second);
    }
  }
}

void KCudfData::pairwiseConflicting(const std::set<unsigned int>& s) {
  for (auto p = s.begin(); p != s.end(); ++p)
    for (auto q = s.begin(); q != s.end(); ++q)
      if (*p != *q)
        packages[*p]->addConflict(*q);
}

const std::map<unsigned int,Package*>&
KCudfData::getPackages(void) const {
  return packages;
}

const std::set<int>&
KCudfData::bigPackages(void) const {
  return bigPackages_;
}

const std::list<int>&
KCudfData::crtPackages(void) const {
  //return crtPackages_;
	return conPackages_;
}

bool
KCudfData::consistent(void) const {
	unsigned int inst = 0;
	std::set<unsigned int> done;
	
  for ( auto p = packages.begin(); p != packages.end(); ++p) {
    Package *pi = p->second;
    unsigned int pi_id = pi->getId();
    if (pi->isConcrete() && done.count(pi_id) == 0) {
      Package *rp = packages.at(pi->getId());
      assert(rp->isConcrete());
      SelfPackage *pk = static_cast<SelfPackage*> (rp);
      if (pk->markedInstall()) {
        inst++;
        //std::cout << "concrete package " << pk->name() << "  " << pk->getVersion() << std::endl;
        bool dep_cons = true;
        for (unsigned int d : pk->getDependencies()) {
          unsigned int pvds = installedProviders(d);
          if (pvds == 0) {
            /*
              std::cout << "pkg-installed " << pk->name() << " " << pk->getVersion()
              << " missing provider for dep " << d << std::endl;
            */
            dep_cons = false;
          }		
        }
        if (dep_cons) {
          bool cnf_cons = true;
          for (unsigned int c: pk->getConflicts()) {
            unsigned int pvds = installedProviders(c);
            if (pvds != 0) {
              /*
                std::cout << "pkg-installed " << pk->name() << " " << pk->getVersion()
                << " installed conflicting " << c << std::endl;
              */	
              cnf_cons = false;
            }		
          }
          if (cnf_cons) {
            conPackages_.push_back(pk->getId());
          }				
        }
      }
      done.insert(rp->getId());
    }
  }
  std::cout << "Total installed packages: " << inst << std::endl
            << "Consistent packages: " << conPackages_.size() << std::endl;
  
  return inst == conPackages_.size();
}

unsigned int
KCudfData::installedProviders(unsigned int d) const {
  Package *p = packages.at(d);
  if (p->isConcrete()) {
    return p->markedInstall() ? 1U : 0U;
  }
  unsigned int c = 0;
  Disjunction *dj = static_cast<Disjunction*>(p);
  for (unsigned int pv : dj->getProviders()) {
		assert(packages.at(pv)->isConcrete());
		if (packages.at(pv)->markedInstall())
			c++;
	}
	return c;
}

void Package::
toStream(std::ostream& o, const std::map<unsigned int, Package*>& packages) const {
  o << "id: " << getId();
  o << " rv: " << getVersion();
  o << " d:{";
  for (int d: dependencies) {
    o << packages.find(d)->second->getId() << " ";
  }
  o << "}#" << dependencies.size();
  o << " c:{";
  for (int c: conflicts) {
    o << packages.find(c)->second->getId() << " ";
  }
  o << "}#"<< conflicts.size();
  // output keep and install
  o << " keep:" << (markedKeep() ? "yes" : "no") << ", install:" << (markedInstall() ? "yes" : "no");
  o << " concrete: " << (isConcrete() ? "yes" : "no");
  if (!isConcrete()) {
    const Disjunction *d = static_cast<const Disjunction*>(this);
    o << " pvded:{";
    for (int p : d->getProviders()) {
      o << packages.find(p)->second->getId() << " ";
    }
    o << "}#" << d->getProviders().size();
    if (d->hasBut()) {
      o << " but: " << d->but();
    }
  }
}

std::ostream& operator<< (std::ostream& o,const KCudfData& kcudf) {
  o << "## Concrete" << std::endl;
  for (auto p = kcudf.concrete.begin(); p != kcudf.concrete.end(); ++p) {
    o << "Name: " << p->first << " size: " << p->second.size() << std::endl;
    for ( auto pi = p->second.begin(); pi != p->second.end(); ++pi) {
      Package *pkg = kcudf.packages.find(pi->second)->second;
      o << "\tversion: " << pi->first << " ";
      pkg->toStream(o, kcudf.packages);
      o << std::endl;
    }
    o << std::endl;
  }

  o << "## SpecV" << std::endl;
  for ( auto p = kcudf.specv.begin(); p != kcudf.specv.end(); ++p) {
    o << "Name: " << p->first << std::endl;
    for (auto pi = p->second.begin(); pi != p->second.end(); ++pi) {
      Package *pkg = kcudf.packages.find(pi->second)->second;
      o << "\tversion: " << pi->first << " ";
      pkg->toStream(o, kcudf.packages);
      o << std::endl;
    }
    o << std::endl;
  }

  o << "## ConstV" << std::endl;
  for (auto p = kcudf.constv.begin(); p != kcudf.constv.end(); ++p) {
    o << "Constr " << p->first << "  :";
    Package *pkg = kcudf.packages.find(p->second)->second;
    if (pkg->isConcrete())
      o << " --fwd--> " << pkg->getId();
    else
      pkg->toStream(o,kcudf.packages);
    o << std::endl;
  }

  return o;
}

/*
 * DTNode
 *
 */
DTNode::DTNode(void) : tree_node(0), cmp(false), children() {}

DTNode::~DTNode(void) {
  for (auto n = children.begin(); n != children.end(); ++n)
    delete n->second;
}

bool DTNode::computed(void) const {
  return cmp;
}

void DTNode::setNode(unsigned int n) {
  tree_node = n;
  cmp = true;
}

unsigned int DTNode::getNode(void) const {
  assert(computed());
  return tree_node;
}

bool DTNode::hasChild(unsigned int u) const {
  return children.find(u) != children.end();
}

DTNode* DTNode::getChild(unsigned int u) {
  return children.find(u)->second;
}

void DTNode::addChild(unsigned int u, DTNode* c) {
  assert(!hasChild(u));
  children[u] = c;
}

unsigned int DTNode::addDisjunction(unsigned int id, const std::set<unsigned int>& pvds) {

  std::set<unsigned int> e(pvds);
  DTNode* curr = this;
  DTNode* parent;
  unsigned int curr_node;

  while (!e.empty()) {
    curr_node = *(e.begin());
    if (curr->hasChild(curr_node)) {
      // there is a path to the current element in the disjunction tree
      curr = curr->getChild(curr_node);
    } else {
      parent = curr;
      curr = new DTNode();
      parent->addChild(curr_node, curr);
    }
    e.erase(e.begin());
  }
  if (curr->computed()) {
    return curr->getNode();
  } else {
    curr->setNode(id);
    return id;
  }
}

/*
 * KCudfTranslator
 */

KCudfTranslator::KCudfTranslator(const CudfDoc& d)
  : doc(d), st(), data(doc,st) {}

const TranslatorStats& KCudfTranslator::stats(void) const {
  return st;
}

void KCudfTranslator::writePackages(KCudfWriter& wrt, KCudfInfoWriter& inf, bool debug) {
  std::set<unsigned int> done;
  auto packages = data.getPackages();
  // concrete packages
  for (auto p = packages.begin(); p != packages.end(); ++p) {
    Package *pi = p->second;
    unsigned int pi_id = pi->getId();
    if (pi->isConcrete() && done.count(pi_id) == 0) {
      Package *rp = packages.at(pi->getId());
      assert(rp->isConcrete());
      SelfPackage *pk = static_cast<SelfPackage*> (rp);
      std::ostringstream desc;
      desc << pk->getVersion() << pk->name();
      wrt.package(pk->getId(), pk->markedKeep(), pk->markedInstall(), desc.str().c_str());
      inf.package(pk->getId(), pk->getVersion(), pk->name().c_str());
      done.insert(rp->getId());
    }
  }
  // artificial packages
  for (auto p = packages.begin(); p != packages.end(); ++p) {
    Package *pi = p->second;
    unsigned int pi_id = pi->getId();

    if (!pi->isConcrete() && done.count(pi_id) == 0) {
      Package *rp = packages.at(pi->getId());
      const char *info = debug ? rp->getInfo() : "";
      wrt.package(rp->getId(), rp->markedKeep(), rp->markedInstall(), info);
      // TODO: this can clash ith a real package version, fix this.
      inf.package(rp->getId(), 999, info);
      done.insert(rp->getId());
    }
  }
}

void KCudfTranslator::writeConcreteSelfProvided(KCudfWriter& wrt, bool debug) {
  std::set<unsigned int> done;
  auto packages = data.getPackages();
  // single disjunctions corresponding to concrete packages
  for (auto p = packages.begin(); p != packages.end(); ++p) {
    Package *pi = p->second;
    unsigned int pi_id = pi->getId();
    if (pi->isConcrete() && done.count(pi_id) == 0) {
      Package *rp = packages.at(pi->getId());
      assert(rp->isConcrete());
      SelfPackage *pk = static_cast<SelfPackage*> (rp);
      std::ostringstream desc;
      if (debug) {
        desc << pk->getVersion() << pk->name() << "-self";
      }
      wrt.provides(pk->getId(), pk->getId(), desc.str().c_str());
      done.insert(rp->getId());
    }
  }
}

void KCudfTranslator::writeDependencies(KCudfWriter& wrt, bool debug) {
  std::set<unsigned int> done;
  auto packages = data.getPackages();
  for (auto p = packages.begin(); p != packages.end(); ++p) {
    Package *pi = p->second;
    unsigned int pi_id = pi->getId();
    if (done.count(pi_id) == 0) {
      Package *rp = packages.at(pi->getId());
      unsigned int id = rp->getId();
      for  (unsigned int d: rp->getDependencies()) {
        std::ostringstream desc;
        Package *p2 = packages.at(d);
        if (debug) {
          desc << rp->getInfo() << " -> " << p2->getInfo();
        }
        wrt.dependency(id, p2->getId(), desc.str().c_str());
      }
      done.insert(rp->getId());
    }
  }
}

void KCudfTranslator::writeConflicts(KCudfWriter& wrt, bool debug) {
  std::set<unsigned int> done;
  auto packages = data.getPackages();
  for (auto p = packages.begin(); p != packages.end(); ++p) {
    Package *pi = p->second;
    unsigned int pi_id = pi->getId();
    if (done.count(pi_id) == 0) {
      Package *rp = packages.at(pi->getId());
      unsigned int id = rp->getId();
      for (unsigned int d: rp->getConflicts()) {
        std::ostringstream desc;
        Package *p2 = packages.at(d);
        /*
          Just to make the output easy to debug, the smaller id is put first. This
          does not have impact on the conflict relation since it is undirected.
        */
        if(id < p2->getId()) {
          if (debug)
            desc << rp->getInfo() << " -- " << p2->getInfo();
          wrt.conflict(id, p2->getId(), desc.str().c_str());
        } else {
          if (debug)
            desc << p2->getInfo() << " -- " << rp->getInfo();
          wrt.conflict(p2->getId(), id, desc.str().c_str());
        }
      }
      done.insert(rp->getId());
    }
  }
}

void KCudfTranslator::writeProvides(KCudfWriter& wrt, bool debug) {
  /**
   * This procedure will call the writer in the following way:
   * wrt.provided(I J C)
   * Where I and J are numeric packages identifiers and C is a possible empty string
   * with meaningless information.
   *
   * The semantic is: "Package I _Provides_ J"
   */
  std::set<unsigned int> done;
  auto packages = data.getPackages();
  for (auto p = packages.begin(); p != packages.end(); ++p) {
    Package *pi = p->second;
    unsigned int pi_id = pi->getId();
    if (!pi->isConcrete() && done.count(pi_id) == 0) {
      Disjunction *rp = static_cast<Disjunction*>(packages.at(pi->getId()));
      unsigned int id = rp->getId();
      for (unsigned int d: rp->getProviders()) {
        std::ostringstream desc;
        Package *p2 = packages.at(d);
        if (debug)
          desc << rp->getInfo() << " -> " << p2->getInfo() ;
        wrt.provides(p2->getId(), id, desc.str().c_str());
        wrt.dependency(p2->getId(), id, desc.str().c_str());
      }
      done.insert(rp->getId());
    }
  }
}

void KCudfTranslator::translate(KCudfWriter& wrt, KCudfInfoWriter& inf, bool dbg) {
  writePackages(wrt, inf, dbg);
  writeDependencies(wrt, dbg);
  writeConflicts(wrt, dbg);
  writeConcreteSelfProvided(wrt, dbg);
  writeProvides(wrt, dbg);
}

void KCudfTranslator::
extraParanoid(std::list<int>& search) const {
  
  std::map<std::string,boost::tuple<bool,std::list<int> > > families;
  std::set<unsigned int> done;
  auto packages = data.getPackages();
  for (auto p = packages.begin(); p != packages.end(); ++p) {
    Package *pi = p->second;
    unsigned int pi_id = pi->getId();
    if (pi->isConcrete() && done.count(pi_id) == 0) {
      Package *rp = packages.at(pi->getId());
      SelfPackage *pk = static_cast<SelfPackage*> (rp);
      families[pk->name()].get<1>().push_back(pk->getId());
      if (pk->markedInstall()) {
        // one of the familiy is marked
        families[pk->name()].get<0>() = true;
      }
      done.insert(rp->getId());
    }
  }
  
  /// output families
  for (auto f = families.begin(); f != families.end(); ++f) {
    if (f->second.get<0>()) {
      //std::cout << "Family: " << f->first << std::endl;
      const std::list<int>& l = f->second.get<1>();
      for (auto v = l.begin(); v != l.end(); ++v) {
        Package *rp = packages.at(*v);
        SelfPackage *pk = static_cast<SelfPackage*> (rp);
        
        if (! pk->markedKeep() && ! pk->markedInstall()) {
          // The package is CU but belongs to a familly of packages in which
          // at least one is installed. In this way and according to the paranoid
          // optimization criteria this package becomes part of the search.
          search.push_back(*v);
          //std::cout << v->name() << ",";
        }
      }
      //std::cout << std::endl;
    }
  }
}

const std::set<int>&
KCudfTranslator::bigInstalled(void) const {
  return data.bigPackages();
}

const std::list<int>&
KCudfTranslator::crtInstalled(void) const {
  return data.crtPackages();
}

void KCudfTranslator::writeParanoid(std::ostream& big) const {
  std::list<int> search;
  extraParanoid(search);
  for(int i: search) {
    big << i << std::endl;
  }
}

void read(std::istream& input, KCudfWriter& wrt) {
  using namespace std;

  if (input.fail())
    throw FailedStream("unable to open stream for reading");

  char inst, keep;
  unsigned int id, id2;
  unsigned int ln = 0; // line number

  std::string line;
  while(input.good()) {
    std::getline(input, line);
    ln++;
    if (line.empty()) continue;
    char t = line[0];
    line.erase(0, 1);
    stringstream ss(line);
    switch (t) {
    case 'P':
      ss >> id; ss >> keep; ss >> inst;
      wrt.package(id,
                  (keep == 'K' ? true : false),
                  (inst == 'I' ? true : false), "");
      // Make explicit a self dependency for all the packages
      wrt.dependency(id, id, "self-dep");
      break;
    case 'D':
      ss >> id; ss >> id2;
      wrt.dependency(id, id2, "");
      break;
    case 'C':
      ss >> id; ss >> id2;
      wrt.conflict(id, id2, "");
      break;
    case 'R':
      ss >> id; ss >> id2;
      wrt.provides(id, id2, "");
      break;
    case '#':
      // just to allow comments starting with #
      break;
    default:
      std::ostringstream ss;
      ss << "Unknown statement found while reading line: ..." << line
          << std::endl
          << " at line: " << ln;
      throw KCudfReaderInvalidStatement(ss.str().c_str());
      break;
      }
    }
}

void readInfo(const char* info,
              std::map<std::string, std::map<unsigned int, unsigned int> >& m) {
  // TODO: change this to use the next function
  using namespace std;
  std::string name;
  unsigned int id, version;

  ifstream file(info);
  std::string l;
  while(std::getline(file, l)) {
    stringstream ss(l);
    ss >> id; ss >> version; ss >> name;
    m[name][version] = id;
  }
}

void readInfo(const char* info, KCudfInfoWriter& wrt) {
  using namespace std;
  std::string name;
  unsigned int id, version;

  ifstream file(info);
  std::string l;
  while(std::getline(file, l)) {
    stringstream ss(l);
    ss >> id; ss >> version; ss >> name;
    wrt.package(id, version, name.c_str());
  }
}

/*
 * CudfUpdater
 */
CudfUpdater::
CudfUpdater(CudfDoc& doc, 
            std::map<std::string,std::map<unsigned int, unsigned int> >& m) 
: changed(0) {
  for (auto pi = doc.pkg_mbegin(); pi != doc.pkg_mend(); ++pi)
    status[m[pi->name()][pi->version()]] = &(*pi);
}

void CudfUpdater::package(unsigned int id, bool keep, bool install, const char*) {
  //std::cerr << "Updating " << id << " " << (install ? "true" : "false") << std::endl;
  if(status.count(id) > 0) {
    CudfPackage * pk = status[id];
    if (pk->installed() != install) {
      changed++;
    }
    pk->install(install);
  } else {
    //std::cerr << "Assuming virtual: " << id << std::endl;
  }
}

void CudfUpdater::dependency(unsigned int, unsigned int, const char*) {}

void CudfUpdater::conflict(unsigned int, unsigned int, const char*) {}

void CudfUpdater::provides(unsigned int, unsigned int, const char*) {}

unsigned int CudfUpdater::stats(void) const {
  return changed;
}

void update(CudfDoc& doc, const char* info, std::ifstream& kcudf0, std::ifstream& kcudf1) {
  std::map<std::string, std::map<unsigned int, unsigned int> > m;
  readInfo(info, m);
  CudfUpdater up(doc,m);

  read(kcudf0,up);
  read(kcudf1,up);
}

/*
 * KCudfInfoMapWriter
 */
KCudfInfoMapWriter::KCudfInfoMapWriter(void)
  : KCudfInfoWriter() {}

KCudfInfoMapWriter::~KCudfInfoMapWriter(void) {}

void KCudfInfoMapWriter::
package(unsigned int id, unsigned int version, const char* name) {
  names[id] = name;
  versions[id] = version;

}

const KCudfInfoMapWriter::id_to_name_t&
KCudfInfoMapWriter::getNames(void) const {
  return names;
}

const KCudfInfoMapWriter::id_to_version_t&
KCudfInfoMapWriter::getVersions(void) const {
  return versions;
}

/*
 * KCudfWriter
 */

KCudfWriter::KCudfWriter(void) {}

KCudfWriter::~KCudfWriter(void) {}

void KCudfWriter::package(unsigned int, bool, bool, const char*) {}

void KCudfWriter::dependency(unsigned int, unsigned int, const char*) {}

void KCudfWriter::conflict(unsigned int, unsigned int, const char*) {}

void KCudfWriter::provides(unsigned int, unsigned int, const char*) {}

/*
 * KCudfInfoWriter
 */
KCudfInfoWriter::KCudfInfoWriter(void) {}

KCudfInfoWriter::~KCudfInfoWriter(void) {}

void KCudfInfoWriter::package(unsigned int, unsigned int, const char*) {}
