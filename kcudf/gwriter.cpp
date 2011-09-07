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

#include <kcudf/gwriter.hh>

using namespace std;


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


/*
 * GraphWriter
 */
GraphWriter::GraphWriter(unsigned int start)
  : KCudfWriter(), c(start) {}

GraphWriter::~GraphWriter(void) {}

void GraphWriter::package(unsigned int id, bool keep, bool install, const char*) {
  // Conceptually all the graphs have the same set of nodes

  ;

  nodes_tuple_t nodes = make_tuple(Digraph::add_vertex(c,id,deps),
                                   Ugraph::add_vertex(c,id,confs),
                                   Digraph::add_vertex(c,id,pvds));

  nodesm.insert(nodesm.end(),make_pair(id,nodes));

  c++;

  node_state_t st = make_tuple(keep,install);
  statem.insert(statem.end(),make_pair(id,st));
}

bool GraphWriter::isPackage(unsigned int p) const {
  return nodesm.count(p) > 0;
}

void GraphWriter::dependency(unsigned int id, unsigned int id2, const char*) {
  assert(isPackage(id));
  assert(isPackage(id2));
  add_edge(get<0>(nodesm.at(id)),
           get<0>(nodesm.at(id2)),
           deps);
}

void GraphWriter::conflict(unsigned int id, unsigned int id2, const char*) {
  assert(isPackage(id));
  assert(isPackage(id2));
  add_edge(get<1>(nodesm.at(id)),
           get<1>(nodesm.at(id2)),
           confs);
}

void GraphWriter::provides(unsigned int id, unsigned int id2, const char*) {
  assert(isPackage(id));
  assert(isPackage(id2));
  add_edge(get<2>(nodesm.at(id)),
           get<2>(nodesm.at(id2)),
           pvds);
}

/*
 * Packages
 */

unsigned int GraphWriter::numPackages(void) const {
  // all the graphs contains the same set of nodes
  return num_vertices(deps);
}

GraphWriter::package_range GraphWriter::packages(void) const {
  // as all the graphs have the same nodes (identifiers) this method relies on the
  // dependency graph to return an iterator on all the registered packages.
  return make_vertex_label_iterator<Digraph>(deps);
}

bool GraphWriter::install(unsigned int p) const {
  return get<1>(statem.at(p));
}

bool GraphWriter::keep(unsigned int p) const {
  return get<0>(statem.at(p));
}

tuple<bool,bool> GraphWriter::state(unsigned int p) const {
  assert(isPackage(p));
  return statem.at(p);
}

void GraphWriter::state(unsigned int p, bool keep, bool install) {
  assert(isPackage(p));
  statem[p] = make_tuple(keep,install);
}

unsigned int GraphWriter::internalId(unsigned int p) const {
  // the three different nodes representing p in the three graphs store the same
  // internal identifier
  using namespace boost;
 assert(get(vertex_index,deps)[get<0>(nodesm.at(p))] == get(vertex_index,confs)[get<1>(nodesm.at(p))]);
 assert(get(vertex_index,deps)[get<0>(nodesm.at(p))] == get(vertex_index,pvds)[get<2>(nodesm.at(p))]);

 return get(vertex_index,deps)[get<0>(nodesm.at(p))];
}

/*
 * Dependencies
 */
unsigned int GraphWriter::numDependencies(void) const {
  return num_edges(deps);
}

unsigned int GraphWriter::numDependencies(unsigned int p) const {
  return out_degree(get<0>(nodesm.at(p)),deps);
}

unsigned int GraphWriter::numDependers(unsigned int p) const {
  return in_degree(get<0>(nodesm.at(p)),deps);
}

GraphWriter::dependency_range GraphWriter::dependencies(unsigned int p) const {
  assert(isPackage(p));
  return make_out_edge_label_iterator<Digraph>(get<0>(nodesm.at(p)),deps);
}

GraphWriter::depender_range GraphWriter::dependers(unsigned int p) const {
  assert(isPackage(p));
  return make_in_edge_label_iterator<Digraph>(get<0>(nodesm.at(p)),deps);
}

//GraphWriter::dependencies_range GraphWriter::dependencies(void) const {
//  assert(false);
//  //  return make_edge_label_iterator<Digraph>(deps);
//}

bool GraphWriter::dependency(unsigned int p, unsigned int q) const {
  assert(isPackage(p));
  for (unsigned int d : dependencies(p))
    if (d == q) return true;
  return false;
}
/*
 * Conflicts
 */
unsigned int GraphWriter::numConflicts(void) const {
  return num_edges(confs);
}

unsigned int GraphWriter::numConflicts(unsigned int p) const {
  return out_degree(get<1>(nodesm.at(p)),confs);
}

GraphWriter::conflict_range GraphWriter::conflicts(unsigned int p) const {
  assert(isPackage(p));
  return make_out_edge_label_iterator<Ugraph>(get<1>(nodesm.at(p)),confs);
}


//pair<GraphWriter::conflicts_iterator,GraphWriter::conflicts_iterator>
//GraphWriter::conflicts(void) const {
//  typedef Ugraph::edge_iterator it_t;
//  it_t begin, end;
//  tie(begin,end) = edges(confs);
//  return make_pair(make_trans_edge_iterator(begin,confs),
//                   make_trans_edge_iterator(end,confs));
//}

bool GraphWriter::conflict(unsigned int p, unsigned int q) const {
  assert(isPackage(p));
  for (unsigned int c : conflicts(p))
    if (c == q) return true;
  return false;
}

/*
 * Provides
 */

unsigned int GraphWriter::numProvides(void) const {
  return num_edges(pvds);
}

unsigned int GraphWriter::numProvides(unsigned int p) const {
  return out_degree(get<2>(nodesm.at(p)),pvds);
}

unsigned int GraphWriter::numProviders(unsigned int p) const {
  return in_degree(get<2>(nodesm.at(p)),pvds);
}

GraphWriter::provide_range GraphWriter::provides(unsigned int p) const {
  assert(isPackage(p));
  return make_out_edge_label_iterator<Digraph>(get<2>(nodesm.at(p)),pvds);
}

GraphWriter::provider_range GraphWriter::providers(unsigned int p) const {
  assert(isPackage(p));
  return make_in_edge_label_iterator<Digraph>(get<2>(nodesm.at(p)),pvds);
}

//pair<GraphWriter::provider_iterator,GraphWriter::provider_iterator>
//GraphWriter::providers(unsigned int p) const {
//  assert(isPackage(p));
//  typedef Digraph::in_edge_iterator it_t;
//  it_t begin, end;
//  tie(begin,end) = in_edges(get<2>(nodesm.at(p)),pvds);
//  return make_pair(v_iterator<rel_first,it_t,Digraph::type>::type(make_trans_edge_iterator(begin,pvds)),
//                   v_iterator<rel_first,it_t,Digraph::type>::type(make_trans_edge_iterator(end,pvds)));
//}

//pair<GraphWriter::provides_iterator,GraphWriter::provides_iterator>
//GraphWriter::provides(void) const {
//  typedef Digraph::edge_iterator it_t;
//  it_t begin, end;
//  tie(begin,end) = edges(pvds);
//  return make_pair(make_trans_edge_iterator(begin,pvds),
//                   make_trans_edge_iterator(end,pvds));
//}

bool GraphWriter::provides(unsigned int p, unsigned int q) const {
  assert(isPackage(p));
  for (unsigned int v : provides(p))
    if (v == q) return true;
  return false;
}

/*
 * Graph access
 */

Digraph::type& GraphWriter::getDeps(void) { return deps; }

const Digraph::type& GraphWriter::getDeps(void) const { return deps; }

Ugraph::type& GraphWriter::getConfs(void) { return confs; }

Digraph::type& GraphWriter::getPvds(void) { return pvds; }
