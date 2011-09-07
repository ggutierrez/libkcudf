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

#ifndef __KCUDF_GWRITER__HH__
#define __KCUDF_GWRITER__HH__

#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/iterator/iterator_adaptor.hpp>
#include <boost/iterator/transform_iterator.hpp>

#include <tuple>
#include <kcudf/kcudf.hh>
#include <kcudf/graph.hh>

class GraphWriter : public KCudfWriter {
private:
  /// Type for the same _conceptual_ node in the three graphs
  typedef
  std::tuple<Digraph::vertex,Digraph::vertex,Ugraph::vertex>
  nodes_tuple_t;
  /// Type for the state of a package (keep, install)
  typedef std::tuple<bool,bool> node_state_t;
protected:
  /**
   * \brief Type for the map that stores the package identifier and their
   * verties in the dependency, conflict and provides graph
   */
  typedef std::map<unsigned int, nodes_tuple_t> nodes_map_t;
  /**
   * \brief Type for the map that stores the package identifier and its state
   */
  typedef std::map<unsigned int, node_state_t> nodes_state_t;
  /// vertex id counter
  unsigned int c;
  /// Dependencies graph
  Digraph::type deps;
  /// Conflicts graph
  Ugraph::type confs;
  /// Providers graph
  Digraph::type pvds;
  /// Nodes in the three graphs
  nodes_map_t nodesm;
  /// State of the packages
  nodes_state_t statem;
public:
  /// \name Iterator types returned by methods of this class
  //@{
  /// node information digraph
  typedef vit_adaptor<Digraph,Digraph::vertex_iterator,vit_extract<Digraph>::label>::type
      info_range;
  /// source information digraph
  typedef
      vit_adaptor<Digraph,Digraph::in_edge_iterator,vit_extract<Digraph>::source_info>::type
      in_source_info_range;
  /// target information digraph
  typedef
      vit_adaptor<Digraph,Digraph::out_edge_iterator,vit_extract<Digraph>::target_info>::type
      out_target_info_range;
  /// edge information digraph
  typedef
      vit_adaptor<Digraph,Digraph::edge_iterator,vit_extract<Digraph>::target_info>::type
      edge_info_range;
  /// target information ugraph
  typedef
      vit_adaptor<Ugraph,Ugraph::out_edge_iterator,vit_extract<Ugraph>::target_info>::type
      out_target_info_range_ud;
  /// edge information ugraph
  typedef
      vit_adaptor<Ugraph,Ugraph::edge_iterator,vit_extract<Ugraph>::target_info>::type
      edge_info_range_ud;

  /// Type for range on dependencies
  typedef out_target_info_range dependency_range;
  /// Type for range on dependers
  typedef in_source_info_range depender_range;
  /// Type for range on provides
  typedef out_target_info_range provide_range;
  /// Type for range of providers
  typedef in_source_info_range provider_range;
  /// Type for range of conflicts
  typedef out_target_info_range_ud conflict_range;
  /// Type for range on dependencies
  typedef edge_info_range dependencies_range;
  /// Type for range on provides
  typedef edge_info_range provides_range;
  /// Type for range on conflicts
  typedef edge_info_range_ud conflicts_range;
  /// Type for range of packages
  typedef info_range  package_range;
  //@}
  /// Package status type
  typedef node_state_t package_state;
  /// Relation between packages type
  typedef std::pair<unsigned int, unsigned int> rel_type;
public:
  /// \name Construcotrs
  //@{
  /**
   * \brief Constructor for a graph writer.
   *
   * \a start represents theinitial value of the internal mapping identifier
   * associated for each node. For most graph algorithms, this must be
   * 0 (default value), however it can be changed for other purposes.
   */
  GraphWriter(unsigned int start = 0);
  /// Destructor
  virtual ~GraphWriter(void);
  //@}
  /// \name Writer implementation
  //@{
  /// Process package \a p
  void package(unsigned int p, bool keep, bool install, const char*);
  /// Process dependency between packages \a p and \a q
  void dependency(unsigned int p, unsigned int q, const char*);
  /// Process conflict between packages \a p and \a q
  void conflict(unsigned int p, unsigned int q, const char*);
  /// Process provides between packages \a p and \a q
  void provides(unsigned int p, unsigned int q, const char*);
  //@}
  /// \name Package information
  //@{
  /// Number of packages
  unsigned int numPackages(void) const;
  /**
   * \brief Iterator on all registered packages
   *
   * \warning Complexity: O(1)
   */
  package_range packages(void) const;
  /// Test whether \a p is a registered package
  bool isPackage(unsigned int p) const;
  /// Tests the install flag of package \a p
  bool install(unsigned int p) const;
  /// Tests the keep flag of package \a p
  bool keep(unsigned int p) const;
  /// Returns the current state of package \a p as a tuple <keep,install>
  std::tuple<bool,bool> state(unsigned int p) const;
  /// Returns the current state of package \a p as a tuple <keep,install>
  void state(unsigned int p, bool keep, bool install);
  /**
   * \brief Returns the internal identifier associated with package \a p.
   *
   * This value is in the range start to |packages| - 1. Where start is the value
   * given to the constructor (0 by default). This value is used by several boost
   * algorithms but I have found it also useful in other contexts (solver).
   */
  unsigned int internalId(unsigned int p) const;
  //@}
  /// \name Dependency information
  //@{
  /**
   * \brief Number of dependency relations
   *
   * \warning Complexity: O(1)
   */
  unsigned int numDependencies(void) const;
  /**
   * \brief Number of dependencies of package \a p
   *
   * \warning Complexity: O(1)
   */
  unsigned int numDependencies(unsigned int p) const;
  /**
   * \brief Number of dependers of package \a p
   *
   * \warning Complexity: O(1)
   */
  unsigned int numDependers(unsigned int p) const;
  /**
   * \brief Dependencies of package \a p
   *
   * \warning Complexity: O(log |V|)
   */
  dependency_range dependencies(unsigned int p) const;
  /**
   * \brief Dependers of package \a p
   *
   * \warning Complexity: O(log |V|)
   */
  depender_range dependers(unsigned int p) const;
  /**
   * \brief Dependencies of all the packages
   *
   * \warning Complexity: O(|E||V|)
   */
  dependencies_range dependencies(void) const;
  /**
   * \brief Tests whether there is a dependency between packages \a p and \a q
   *
   * \warning Complexity: O(|E| * log |V|)
   */
  bool dependency(unsigned int p, unsigned int q) const;
  //@}
  /// \name Conflict information
  //@{
  /// Number of conflicts
  unsigned int numConflicts(void) const;
  /**
   * \brief Number of conflicts of package \a p
   *
   * \warning Complexity: O(log |V|)
   */
  unsigned int numConflicts(unsigned int p) const;
  /**
   * \brief Conflicts of package \a p
   *
   * \warning Complexity: O(log |V|)
   */
  conflict_range conflicts(unsigned int p) const;
  /**
   * \brief Conflicts of all the packages
   *
   * \warning Complexity: O(|E||V|)
   */
//  std::pair<conflicts_iterator,conflicts_iterator>
//      conflicts(void) const;
  /**
   * \brief Tests whether there is a conflict between packages \a p and \a q
   *
   * \warning Complexity: O(|E| * log |V|)
   */
  bool conflict(unsigned int p, unsigned int q) const;
  //@}
  /// \name Provides information
  //@{
  /// Number of provides
  unsigned int numProvides(void) const;
  /**
   * \brief Number of provides of package \a p
   *
   * \warning Complexity: O(1)
   */
  unsigned int numProvides(unsigned int p) const;
  /**
   * \brief Number of providers of package \a p
   *
   * \warning Complexity: O(1)
   */
  unsigned int numProviders(unsigned int p) const;
  /**
   * \brief Provides of package \a p
   *
   * \warning Complexity: O(log |V|)
   */
  provide_range provides(unsigned int p) const;
  /**
   * \brief Provides of package \a p
   *
   * \warning Complexity: O(log |V|)
   */
  provider_range providers(unsigned int p) const;
  /**
   * \brief Provides of all the packages
   *
   * \warning Complexity: O(|E||V|)
   */
//  std::pair<provides_iterator,provides_iterator>
//      provides(void) const;
  /**
   * \brief Tests whether package \a p provides package \a q
   *
   * \warning Complexity: O(|E| * log |V|)
   */
  bool provides(unsigned int p, unsigned int q) const;
  //@}
  /// \name Graph access
  //@{
  /// Return the graph of dependencies
  Digraph::type& getDeps(void);
  const Digraph::type& getDeps(void) const;
  /// Return the graph of conflicts
  Ugraph::type& getConfs(void);
  /// Return the graph of provides
  Digraph::type& getPvds(void);
  //@}
};

#endif
