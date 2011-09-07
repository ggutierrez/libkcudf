/* -*- mode: C++; c-basic-offset: 2; indent-tabs-mode: nil -*- */
/*
 *  Main authors:
 *     Gustavo Gutierrez <gutierrez.gustavo@uclouvain.be>
 *
 *  Copyright:
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

#ifndef __CMD_OPTIONS_HH__
#define __CMD_OPTIONS_HH__

#include <iostream>
#include <boost/program_options.hpp>

/**
 * \brief Helper function to define that the option \a opt is mandatory
 * in the variables map \a vm.
 */
inline void 
mandatory_option(const boost::program_options::variables_map& vm, const char* opt) {
  if (vm.count(opt) == 0 || vm[opt].defaulted()) {
    std::cerr << "Mandatory option '" << opt << "'." 
              << std::endl << "use --help for more info." << std::endl;
    exit(EXIT_FAILURE);
  }
}

/**
 * \brief Check whether option \a opt is enabled or not.
 */
inline bool 
optionEnabled(const boost::program_options::variables_map& vm, const char* opt) {
  if (vm.count(opt) && !vm[opt].defaulted())
    return true;
  return false;
}
#endif
