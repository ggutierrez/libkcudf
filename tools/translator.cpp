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
#include <kcudf/kcudf.hh>
#include <kcudf/swriter.hh>
#include "cmd-options.hh"

using namespace boost::program_options;

void parseCmdOptions(variables_map& vm, int argc, char* argv[]) {
  options_description general("Available options");

   general.add_options()
     ("cudf", value<std::string>(),
      "File containing the cudf description.\n")
     ("kcudf", value<std::string>(),
      "File containing the resulting kernel cudf")
     ("info", value<std::string>(),
      "File containing the info file")
     ("paranoid", value<std::string>(),
      "File to ouput paranoid related information")
     ("dumpdb", value<std::string>(),
      "File that will contain the database commands")
     ("debug", bool_switch(),"Include debug information, useful for the dotter but on big inputs it can be slow.\n")
     ("help", "print this message");
   
   positional_options_description pd;
   pd.add("cudf",1).add("kcudf",1).add("info",1);

   store(command_line_parser(argc, argv).options(general).positional(pd).run(), vm);
   notify(vm);

  if (vm.count("help")) {
    std::cout << std::endl << "Example calls:" << std::endl
    << argv[0] << " input.cudf " << std::endl
    << argv[0] << " input.cudf out.kcudf inf.txt" << std::endl
    << argv[0] << " --cudf input.cudf --kcudf out.kcudf --info inf.txt" << std::endl
    << std::endl << std::endl << general << std::endl;
    exit(EXIT_SUCCESS);
   }
}

void writeStats(std::ostream& os, const TranslatorStats& st) {
  if (st.fail) {
    os << "No solution" << std::endl;
    return;
  }

  os
    << "Translation statistics:" << std::endl
    << "\tConcrete packages: " << st.cp << std::endl
    << "\tReal disjunctions: " << st.rd << std::endl
    << "\tEqual disj: " << st.ed << std::endl
    << "\tZero-provider disj: " << st.zp << std::endl
    << std::endl;
}

int main(int argc, char **argv) {
  using namespace std;

  // command line options
  variables_map vm;
  parseCmdOptions(vm,argc,argv);
  mandatory_option(vm,"cudf");

  const char* input = vm["cudf"].as<std::string>().c_str();
  ifstream cudf_st(input);
  if (!cudf_st) {
    cerr << "error: file '" << input << "' not found" << endl;
    return EXIT_FAILURE;
  }

  std::string kcudfname(input); kcudfname.append(".kcudf");
  std::string infoname(input); infoname.append(".info");

  const char* kcudf = (optionEnabled(vm, "kcudf")) ?
    vm["kcudf"].as<std::string>().c_str() : kcudfname.c_str();
  const char* info = (optionEnabled(vm, "info")) ?
    vm["info"].as<std::string>().c_str() : infoname.c_str();

	CudfDoc doc;
	parse(cudf_st,doc);
  KCudfFileWriter out(kcudf);
  KCudfInfoFileWriter inf(info);


  KCudfTranslator tr(doc);

  try {
    tr.translate(out,inf,vm["debug"].as<bool>());
    //tm.stop();
  } catch (KCudfFailedRequest& fr) {
    std::cerr << fr.what();
  } catch (KCudfInvalidProvide& ip) {
    std::cerr << ip.what();
    exit(EXIT_FAILURE);
  } catch (...) {
    std::cerr << "Unknown exception!" << endl;
    exit(EXIT_FAILURE);
  }
  
  if (optionEnabled(vm,"paranoid")) {
    ofstream os(vm["paranoid"].as<std::string>().c_str());
    if (!os) {
      cerr << "file to ouput extra information cannot be oppened" << endl;
      return EXIT_FAILURE;
    }
    tr.writeParanoid(os);
    os.close();
  }

//  if (optionEnabled(vm,"inst-concretes")) {
//    ofstream os(vm["inst-concretes"].as<std::string>().c_str());
//    if (!os) {
//      cerr << "file to ouput extra information cannot be oppened" << endl;
//      return EXIT_FAILURE;
//    }
//    tr.(os);
//    os.close();
//  }
  
  
  writeStats(std::cerr,tr.stats());

  std::cout << "Generated KCUDF file: " << kcudf << std::endl;
  std::cout << "Generated INFO file: " << info << std::endl;
  return EXIT_SUCCESS;
}
