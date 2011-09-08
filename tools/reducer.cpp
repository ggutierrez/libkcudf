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

#include <iostream>
#include <fstream>
#include <boost/program_options.hpp>
#include "cmd-options.hh"
#include <kcudf/reduce.hh>
#include <kcudf/swriter.hh>
#include <kcudf/gwriter.hh>

using namespace boost::program_options;

using namespace std;

void parseCmdOptions(variables_map& vm, int argc, char* argv[]) {
  options_description general("Available options");

  general.add_options()
    ("kcudf", value<std::string>(),
     "File containing the kcudf description.\n")
    ("solved", value<std::string>(),
     "Solved kcudf (only contains package information).\n")
    ("search", value<std::string>(),
     "Resulting kcudf with the problem instance.\n")
    ("paranoid", value<std::string>(),
     "file to read paranoid data from\n")
    ("dumpdb", value<std::string>(),
     "File that will contain the database commands")
    ("help", "print this message");

  positional_options_description pd;
  pd.add("kcudf",1).add("solved",1).add("search",1);

  store(command_line_parser(argc, argv).options(general).positional(pd).run(), vm);
  notify(vm);

 if (vm.count("help")) {
   cout << endl << "Example calls:" << endl
   << argv[0] << " input.kcudf solved.kcudf prob.kcudf" << endl
   << argv[0] << " --kcudf input.kcudf --solved easy.kcudf --search prob.kcudf" << endl
   << endl << endl << general << endl;
   exit(EXIT_SUCCESS);
  }
}

void writeStats(ostream& os, const ReducerStats& st) {
  if (st.fail) {
    os << "No solution" << endl;
    return;
  }

  os 
    << "Reduction statistics:" << endl
    << "\tPackages in search " << st.pkg_srch << endl
    << "\tOther packages " << st.pkg_is << endl
    << "\tInitial packages " << st.pkgs << endl
    << "\tReduction: " << ((st.pkg_is + st.pkg_srch)*100.0/st.pkgs) << endl;
}

int main(int argc, char **argv) {
  using namespace std;

  // command line options
  variables_map vm;
  parseCmdOptions(vm,argc,argv);
  mandatory_option(vm,"kcudf");
  mandatory_option(vm,"solved");
  mandatory_option(vm,"search");

  const char* kcudf = vm["kcudf"].as<std::string>().c_str();
  ifstream kcudf_st(kcudf);
  if (!kcudf_st) {
    cerr << "error: file '" << kcudf << "' not found" << endl;
    return EXIT_FAILURE;
  }

  const char* solved = vm["solved"].as<std::string>().c_str();
  const char* search = vm["search"].as<std::string>().c_str();
  const char* db = (optionEnabled(vm, "dumpdb")) ? vm["dumpdb"].as<std::string>().c_str() : "";

  KCudfReducer* red;
  if (optionEnabled(vm,"paranoid")) {
    std::ifstream is(vm["paranoid"].as<std::string>().c_str());
    if (!is) {
      cerr << "Cannot open file with paranoid information" << endl;
    }
    red = new KCudfReducer(is);
  } else {
    red = new KCudfReducer;
  }

  
  read(kcudf_st,*red);
  KCudfFileWriter es(solved);
  KCudfFileWriter sr(search);

  cerr << "*** Reducing: " << kcudf << endl
       << "\tsolved:\t" << solved << endl
       << "\tsearch:\t" << search << endl;

  KCudfReducer::RD_OUT rout = red->reduce(es,sr);

  switch (rout) {
    case KCudfReducer::RDO_SOL:
      cerr << "** The reducer has found a solution **" << endl;
      break;
    case KCudfReducer::RDO_FAIL:
      cerr << "** No solution **" << endl;
      break;
    case KCudfReducer::RDO_SEARCH:
      cerr << "** NEED SERCH **" << endl;
      break;
  }

  cerr 
    << "The file " << solved
    << " contains the solved part of the problem" << endl
    << "The file " << search
    << " contains the input for the solver" << endl;

  return EXIT_SUCCESS;
}
